package main

import (
	"sync"
)

func min(a int, b int) int {
	if a < b {
		return a
	} else {
		return b
	}
}

type TreeIdHash struct {
	TreeId int
	Hash   int
}

var globalHashLock sync.Mutex

func hashWorker(trees []*Tree, start int, end int, treesByHash HashGroups,
	treeIdHashes chan<- TreeIdHash, writeMapWithLock bool) {
	for i, tree := range trees[start:end] {
		treeId := i + start
		hash := tree.Hash(1)

		if treeIdHashes != nil {
			treeIdHashes <- TreeIdHash{treeId, hash}
		} else if writeMapWithLock {
			globalHashLock.Lock()
			treesByHash[hash] = append(treesByHash[hash], treeId)
			globalHashLock.Unlock()
		} else {
			// no map insert, just compute
		}
	}
}

//TODO: can memory pre-allocate treesByHash
func hashTreesParallel(trees []*Tree, hashWorkersCount int, dataWorkersCount int) HashGroups {
	var treesByHash = make(HashGroups)
	var hashWorkerWait sync.WaitGroup
	writeMapWithLock := dataWorkersCount == hashWorkersCount
	hashBatchSize := len(trees)/hashWorkersCount + (min(len(trees)%hashWorkersCount, 1))

	var treeIdHashes []chan TreeIdHash
	if dataWorkersCount > 0 && !writeMapWithLock {
		treeIdHashes = make([]chan TreeIdHash, dataWorkersCount)
		for j := range treeIdHashes {
			treeIdHashes[j] = make(chan TreeIdHash, hashWorkersCount*10)
		}
	}

	//start workers
	for i := 0; i < hashWorkersCount; i++ {
		hashWorkerWait.Add(1)

		go func(i int) {
			defer hashWorkerWait.Done()

			start := min(i*hashBatchSize, len(trees))
			end := min((i+1)*hashBatchSize, len(trees))

			if dataWorkersCount == 0 || writeMapWithLock {
				hashWorker(trees, start, end, treesByHash, nil, writeMapWithLock)
			} else {
				hashWorker(trees, start, end, treesByHash, treeIdHashes[i%dataWorkersCount], writeMapWithLock)
			}
		}(i)
	}

	var dataWorkerWait sync.WaitGroup

	//start data workers
	if dataWorkersCount > 0 && !writeMapWithLock {
		for j := 0; j < dataWorkersCount; j++ {
			dataWorkerWait.Add(1)

			go func(j int) {
				defer dataWorkerWait.Done()

				for treeIdHash := range treeIdHashes[j] {
					//TODO: synchronize via one mutex
					//TODO: or synchronize via mutex per hash
					//TODO: or by channel <-> hash parity
					treesByHash[treeIdHash.Hash] = append(treesByHash[treeIdHash.Hash], treeIdHash.TreeId)
				}
			}(j)
		}
	}

	hashWorkerWait.Wait()

	for j := range treeIdHashes {
		close(treeIdHashes[j])
	}

	dataWorkerWait.Wait()

	return treesByHash
}
