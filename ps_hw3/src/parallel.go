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

func hashWorker(trees []*Tree, treeIds <-chan int, treeIdHashes chan<- TreeIdHash) {
	for treeId := range treeIds {
		if treeIdHashes != nil {
			treeIdHashes <- TreeIdHash{treeId, trees[treeId].Hash()}
		} else {
			// no map insert, just compute
			trees[treeId].Hash()
		}
	}
}

func hashTreesParallel(trees []*Tree, hashWorkersCount int, dataWorkersCount int) HashGroups {
	var treesByHash = make(HashGroups)
	var hashWorkerWait sync.WaitGroup
	treeIdsPerWorker := make([]chan int, hashWorkersCount)

	for i := range treeIdsPerWorker {
		treeIdsPerWorker[i] = make(chan int, len(trees)/hashWorkersCount)
	}

	var treeIdHashes chan TreeIdHash

	if dataWorkersCount > 0 {
		treeIdHashes = make(chan TreeIdHash, hashWorkersCount)
	}

	for i := 0; i < hashWorkersCount; i++ {
		hashWorkerWait.Add(1)

		go func(i int) {
			defer hashWorkerWait.Done()

			hashWorker(trees, treeIdsPerWorker[i], treeIdHashes)
		}(i)
	}

	for i, _ := range trees {
		treeIdsPerWorker[i%hashWorkersCount] <- i
	}

	for i := range treeIdsPerWorker {
		close(treeIdsPerWorker[i])
	}

	var dataWorkerWait sync.WaitGroup

	for j := 0; j < dataWorkersCount; j++ {
		dataWorkerWait.Add(1)

		go func() {
			defer dataWorkerWait.Done()

			for treeIdHash := range treeIdHashes {
				treesByHash[treeIdHash.Hash] = append(treesByHash[treeIdHash.Hash], treeIdHash.TreeId)
			}
		}()
	}

	hashWorkerWait.Wait()

	close(treeIdHashes)

	dataWorkerWait.Wait()

	return treesByHash
}
