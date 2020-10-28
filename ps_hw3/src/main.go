//TODO You should submit your code and report packaged into a tar file on Canvas. If you do not complete the optional implementation in step 2.2, name your main go program BST.go. If you do complete it, name it BST_opt.go instead.

//TODO:
//buffer parallel comp
//opt/fine-grained
//unroll hash

package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"runtime/pprof"
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

type ComparisonGroups struct {
	Matrixes map[int]AdjacencyMatrix
	SkipIds  []bool
}

// triangular matrix array of size Size*(Size+1)/2
type AdjacencyMatrix struct {
	Size   int
	Values []bool
}

func (comparisonGroups *ComparisonGroups) Print(hashGroups *HashGroups) {
	group := 0

	for hash, matrix := range comparisonGroups.Matrixes {
		n := matrix.Size
		treeIndexes := (*hashGroups)[hash]

		for i := 0; i < n; i++ {
			if comparisonGroups.SkipIds[treeIndexes[i]] {
				continue
			}

			printedGroup := false

			for j := i + 1; j < n; j++ {
				if matrix.Values[i*(2*n-i+1)/2+j-i] {
					if !printedGroup {
						printedGroup = true
						fmt.Printf("group %v:", group)
						group += 1
					}

					fmt.Printf(" %v", treeIndexes[j])
				}
			}

			if printedGroup {
				fmt.Printf(" %v\n", treeIndexes[i])
			}
		}
	}
}

func main() {
	hashWorkersCount := flag.Int("hash-workers", 1, "integer-valued number of threads")
	dataWorkersCount := flag.Int("data-workers", 0, "integer-valued number of threads")
	compWorkersCount := flag.Int("comp-workers", 0, "integer-valued number of threads")
	inputFile := flag.String("input", "", "input file")
	buffered := flag.Bool("buffered", true, "use a buffered channel for data-worker writes")
	cpuprofile := flag.String("cpuprofile", "", "write cpu profile to file")

	flag.Parse()

	if *inputFile == "" {
		log.Fatal("input file cannot be blank!")
	}

	if *cpuprofile != "" {
		f, err := os.Create(*cpuprofile)
		if err != nil {
			log.Fatal(err)
		}
		pprof.StartCPUProfile(f)
		defer pprof.StopCPUProfile()
	}

	trees := ReadTrees(inputFile)

	start := time.Now()

	var hashGroups HashGroups

	if *hashWorkersCount == 1 {
		hashGroups = hashTreesSequential(trees, *dataWorkersCount == 1)
	} else {
		hashGroups = hashTreesParallel(trees, *hashWorkersCount, *dataWorkersCount, *buffered)
	}

	elapsed := time.Since(start)

	if *dataWorkersCount == 0 {
		fmt.Printf("hashTime: %vµs\n", float64(elapsed.Nanoseconds())/1000)
	} else {
		fmt.Printf("hashGroupTime: %vµs\n", float64(elapsed.Nanoseconds())/1000)
	}

	hashGroups.Print()

	if *compWorkersCount == 0 {
		return
	}

	start = time.Now()

	var comparisonGroups ComparisonGroups

	if *compWorkersCount == 1 {
		comparisonGroups = compareTreesSequential(hashGroups, trees)
	} else if *buffered {
		comparisonGroups = compareTreesParallel(hashGroups, trees, *compWorkersCount)
	} else {
		comparisonGroups = compareTreesParallelUnbuffered(hashGroups, trees)
	}

	elapsed = time.Since(start)

	fmt.Printf("compareTreeTime: %vµs\n", float64(elapsed.Nanoseconds())/1000)

	comparisonGroups.Print(&hashGroups)
}
