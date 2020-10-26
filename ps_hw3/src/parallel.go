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

func hashWorker(trees []*Tree, treesByHash HashGroups,
	treeIds <-chan int, treeIdHashes chan<- TreeIdHash, writeMapWithLock bool) {
	for treeId := range treeIds {
		if treeIdHashes != nil {
			treeIdHashes <- TreeIdHash{treeId, trees[treeId].Hash()}
		} else if writeMapWithLock {
			globalHashLock.Lock()
			hash := trees[treeId].Hash()
			treesByHash[hash] = append(treesByHash[hash], treeId)
			globalHashLock.Unlock()
		} else {
			// no map insert, just compute
			trees[treeId].Hash()
		}
	}
}

//TODO: can memory pre-allocate treesByHash
func hashTreesParallel(trees []*Tree, hashWorkersCount int, dataWorkersCount int) HashGroups {
	var treesByHash = make(HashGroups)
	var hashWorkerWait sync.WaitGroup
	writeMapWithLock := dataWorkersCount == hashWorkersCount

	var treeIdHashes []chan TreeIdHash
	if dataWorkersCount > 0 && !writeMapWithLock {
		treeIdHashes = make([]chan TreeIdHash, dataWorkersCount)
		for j := range treeIdHashes {
			//TODO: multiply by a constant?
			treeIdHashes[j] = make(chan TreeIdHash, hashWorkersCount*10)
		}
	}

	//start workers
	for i := 0; i < hashWorkersCount; i++ {
		hashWorkerWait.Add(1)

		go func(i int) {
			defer hashWorkerWait.Done()

			if dataWorkersCount == 0 || writeMapWithLock {
				hashWorker(trees, treesByHash, treeIdsPerWorker[i], nil, writeMapWithLock)
			} else {
				hashWorker(trees, treesByHash, treeIdsPerWorker[i], treeIdHashes[i%dataWorkersCount], writeMapWithLock)
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
					//TODO: synchronize via mutex
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
