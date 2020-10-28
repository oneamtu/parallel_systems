package main

import (
	"container/list"
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
func hashTreesParallel(trees []*Tree, hashWorkersCount int, dataWorkersCount int, buffered bool) HashGroups {
	var treesByHash = make(HashGroups)
	var hashWorkerWait sync.WaitGroup
	writeMapWithLock := dataWorkersCount == hashWorkersCount
	hashBatchSize := len(trees)/hashWorkersCount + (min(len(trees)%hashWorkersCount, 1))

	var treeIdHashes []chan TreeIdHash
	if dataWorkersCount > 0 && !writeMapWithLock {
		treeIdHashes = make([]chan TreeIdHash, dataWorkersCount)
		for j := range treeIdHashes {
			if buffered {
				treeIdHashes[j] = make(chan TreeIdHash, hashWorkersCount*10)
			} else {
				treeIdHashes[j] = make(chan TreeIdHash)
			}
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

type WorkBuffer struct {
	items     *list.List
	done      bool
	size      int
	lock      *sync.Mutex
	waitFull  *sync.Cond
	waitEmpty *sync.Cond
}

type WorkBufferItem struct {
	matrix *AdjacencyMatrix
	i      int
	treeI  int
	j      int
	treeJ  int
}

func (workBuffer *WorkBuffer) Remove() *WorkBufferItem {
	workBuffer.lock.Lock()
	for workBuffer.items.Len() == 0 && !workBuffer.done {
		workBuffer.waitEmpty.Wait()
	}

	if workBuffer.items.Len() == 0 && workBuffer.done {
		workBuffer.lock.Unlock()
		return nil
	}

	el := workBuffer.items.Back()
	workBuffer.items.Remove(el)

	workBuffer.waitFull.Signal()
	workBuffer.lock.Unlock()

	return el.Value.(*WorkBufferItem)
}

func (workBuffer *WorkBuffer) Add(item *WorkBufferItem) {
	workBuffer.lock.Lock()
	for workBuffer.items.Len() == workBuffer.size {
		workBuffer.waitFull.Wait()
	}

	workBuffer.items.PushBack(item)

	workBuffer.waitEmpty.Signal()
	workBuffer.lock.Unlock()

	return
}

func compareTreesParallel(hashGroup HashGroups, trees []*Tree, compWorkersCount int) ComparisonGroups {
	var compareWorkerWait sync.WaitGroup

	var comparisonGroups ComparisonGroups
	comparisonGroups.Matrixes = make(map[int]AdjacencyMatrix)
	comparisonGroups.SkipIds = make([]bool, len(trees))

	var lock sync.Mutex
	workBuffer := WorkBuffer{
		items:     list.New(),
		done:      false,
		size:      compWorkersCount,
		lock:      &lock,
		waitFull:  sync.NewCond(&lock),
		waitEmpty: sync.NewCond(&lock),
	}

	for i := range comparisonGroups.SkipIds {
		comparisonGroups.SkipIds[i] = false
	}

	for i := 0; i < compWorkersCount; i++ {
		compareWorkerWait.Add(1)

		go func(workBuffer *WorkBuffer) {
			defer compareWorkerWait.Done()

			for item := workBuffer.Remove(); item != nil; item = workBuffer.Remove() {
				if !comparisonGroups.SkipIds[item.treeI] &&
					treesEqualSequential(trees[item.treeI], trees[item.treeJ]) {
					comparisonGroups.SkipIds[item.treeJ] = true
					// println("DEBUG:", treeI, "equal", treeJ)
					item.matrix.Values[item.i*(2*item.matrix.Size-item.i+1)/2+item.j+1] = true
				}
			}
		}(&workBuffer)
	}

	for hash, treeIndexes := range hashGroup {
		if len(treeIndexes) > 1 {
			n := len(treeIndexes)
			matrix := AdjacencyMatrix{n, make([]bool, n*(n+1)/2)}

			for i := range matrix.Values {
				matrix.Values[i] = false
			}

			for i, treeI := range treeIndexes {
				if comparisonGroups.SkipIds[treeI] {
					continue
				}

				for j, treeJ := range treeIndexes[i+1:] {
					workBuffer.Add(&WorkBufferItem{&matrix, i, treeI, j, treeJ})
				}
			}
			comparisonGroups.Matrixes[hash] = matrix
		}
	}

	workBuffer.lock.Lock()

	workBuffer.done = true
	workBuffer.waitEmpty.Broadcast()

	workBuffer.lock.Unlock()

	compareWorkerWait.Wait()

	return comparisonGroups
}

func compareTreesParallelUnbuffered(hashGroup HashGroups, trees []*Tree) ComparisonGroups {
	var compareWorkerWait sync.WaitGroup
	var comparisonGroups ComparisonGroups
	comparisonGroups.Matrixes = make(map[int]AdjacencyMatrix)
	comparisonGroups.SkipIds = make([]bool, len(trees))

	for i := range comparisonGroups.SkipIds {
		comparisonGroups.SkipIds[i] = false
	}

	for hash, treeIndexes := range hashGroup {
		if len(treeIndexes) > 1 {
			n := len(treeIndexes)
			matrix := AdjacencyMatrix{n, make([]bool, n*(n+1)/2)}

			for i := range matrix.Values {
				matrix.Values[i] = false
			}

			for i, treeI := range treeIndexes {
				if comparisonGroups.SkipIds[treeI] {
					continue
				}

				for j, treeJ := range treeIndexes[i+1:] {
					compareWorkerWait.Add(1)

					go func(matrix AdjacencyMatrix, n int, i int, treeI int, j int, treeJ int) {
						defer compareWorkerWait.Done()
						if !comparisonGroups.SkipIds[treeI] && treesEqualSequential(trees[treeI], trees[treeJ]) {
							comparisonGroups.SkipIds[treeJ] = true
							// println("DEBUG:", treeI, "equal", treeJ)
							matrix.Values[i*(2*n-i+1)/2+j+1] = true
						}
					}(matrix, n, i, treeI, j, treeJ)
				}
			}
			comparisonGroups.Matrixes[hash] = matrix
		}
	}

	compareWorkerWait.Wait()

	return comparisonGroups
}
