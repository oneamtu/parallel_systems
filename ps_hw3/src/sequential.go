package main

//TODO: can memory pre-allocate treesByHash
func hashTreesSequential(trees []*Tree, updateMap bool) HashGroups {
	var treesByHash = make(HashGroups)

	for treeId, tree := range trees {
		hash := tree.Hash()
		if updateMap {
			treesByHash[hash] = append(treesByHash[hash], treeId)
		}
	}
	return treesByHash
}

func treesEqualSequential(tree_1 *Tree, tree_2 *Tree) bool {
	var values_1 []int
	var values_2 []int

	tree_1.depthFirstFill(&values_1)
	tree_2.depthFirstFill(&values_2)

	if len(values_1) != len(values_2) {
		return false
	}

	for i, v := range values_1 {
		if v != values_2[i] {
			return false
		}
	}
	return true
}

//TODO: can memory pre-allocate/optimize
//idea 1: pre-allocate new array at treeIndex size (or just max treeIndex & wipe)
func compareTreesSequential(hashGroup HashGroups, trees []*Tree) IdGroups {
	var idGroups IdGroups

	for _, treeIndexes := range hashGroup {
		if len(treeIndexes) > 1 {
			skipIds := make([]bool, len(treeIndexes))
			eqIds := make([]int, 0, len(treeIndexes))

			for i := range skipIds {
				skipIds[i] = false
			}

			for i, treeI := range treeIndexes {

				if skipIds[i] {
					continue
				}

				for j, treeJ := range treeIndexes[i+1:] {
					if treesEqualSequential(trees[treeI], trees[treeJ]) {
						skipIds[j] = true
						eqIds = append(eqIds, treeJ)
					}
				}

				if len(eqIds) > 0 {
					eqIds = append(eqIds, treeI)
					idGroups = append(idGroups, eqIds)
					eqIds = make([]int, 0, len(treeIndexes))
				}
			}
		}
	}
	return idGroups
}
