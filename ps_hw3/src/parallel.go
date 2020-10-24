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

// func hashTree(trees []*Tree, queue <-chan int) {
//   select c <-
// }

func hashTreesParallel(trees []*Tree, hashWorkersCount int, dataWorkersCount int) HashGroups {
	var treesByHash = make(HashGroups)
	var wg sync.WaitGroup
	hashBatchSize := len(trees)/hashWorkersCount + (min(len(trees)%hashWorkersCount, 1))

	for i := 0; i < hashWorkersCount; i++ {
		wg.Add(1)

		go func(i int) {
			defer wg.Done()

			start := min(i*hashBatchSize, len(trees))
			end := min((i+1)*hashBatchSize, len(trees))

			for _, tree := range trees[start:end] {
				tree.Hash()
			}
			// treesByHash[hash] = append(treesByHash[hash], i)
		}(i)
	}

	wg.Wait()

	return treesByHash
}
