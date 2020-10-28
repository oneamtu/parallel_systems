package main

//TODO: can memory pre-allocate treesByHash
func hashTreesSequential(trees []*Tree, updateMap bool) HashGroups {
	var treesByHash = make(HashGroups, 1000) // up to 1000 potential hashes

	for treeId, tree := range trees {
		hash := tree.Hash(1)
		if updateMap {
			treesByHash[hash] = append(treesByHash[hash], treeId)
		}
	}
	return treesByHash
}

func compareTreesSequential(hashGroup HashGroups, trees []*Tree) ComparisonGroups {
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
					if !comparisonGroups.SkipIds[treeJ] && treesEqualSequential(trees[treeI], trees[treeJ]) {
						comparisonGroups.SkipIds[treeJ] = true
						// println("DEBUG:", treeI, "equal", treeJ)
						matrix.Values[i*(2*n-i+1)/2+j+1] = true
					}
				}
			}
			comparisonGroups.Matrixes[hash] = matrix
		}
	}

	return comparisonGroups
}
