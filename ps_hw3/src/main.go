//TODO You should submit your code and report packaged into a tar file on Canvas. If you do not complete the optional implementation in step 2.2, name your main go program BST.go. If you do complete it, name it BST_opt.go instead.

//TODO:
//timing
//seq hash comp
//ruby test/data
//parallel hash
//parallel data
//parallel barrier
//parallel comp
//opt/fine-grained

package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"time"
)

type HashGroups map[int][]int

func (hashGroups *HashGroups) Print() {
	for hash, treeIndexes := range *hashGroups {
		if len(treeIndexes) > 1 {
			fmt.Printf("%v:", hash)
			for _, index := range treeIndexes {
				fmt.Printf(" %v", index)
			}
			fmt.Printf("\n")
		}
	}
}

type IdGroups [][]int

func (idGroups *IdGroups) Print() {
	for id, treeIndexes := range *idGroups {
		fmt.Printf("group %v:", id)
		for _, index := range treeIndexes {
			fmt.Printf(" %v", index)
		}
		fmt.Printf("\n")
	}
}

func main() {
	hashWorkersCount := flag.Int("hash-workers", 1, "integer-valued number of threads")
	dataWorkersCount := flag.Int("data-workers", 0, "integer-valued number of threads")
	compWorkersCount := flag.Int("comp-workers", 0, "integer-valued number of threads")
	inputFile := flag.String("input", "", "input file")

	flag.Parse()

	if *inputFile == "" {
		log.Fatal("input file cannot be blank!")
	}

	trees := ReadTrees(inputFile)

	start := time.Now()

	var hashGroups HashGroups

	if *hashWorkersCount == 1 {
		hashGroups = hashTreesSequential(trees, *dataWorkersCount == 1)
	} else {
		hashGroups = hashTreesParallel(trees, *hashWorkersCount, *dataWorkersCount)
	}

	elapsed := time.Since(start)

	if *dataWorkersCount == 0 {
		fmt.Printf("hashTime: %vµs\n", float64(elapsed.Nanoseconds())/1000)
	} else {
		fmt.Printf("hashGroupTime: %vµs\n", float64(elapsed.Nanoseconds())/1000)
	}

	hashGroups.Print()

	if *compWorkersCount == 0 {
		os.Exit(0)
	}

	start = time.Now()

	var comparisonGroups IdGroups

	if *compWorkersCount == 1 {
		comparisonGroups = compareTreesSequential(hashGroups, trees)
	}

	elapsed = time.Since(start)

	fmt.Printf("compareTreeTime: %vµs\n", float64(elapsed.Nanoseconds())/1000)

	comparisonGroups.Print()
}
