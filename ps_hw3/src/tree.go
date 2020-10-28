package main

import (
	"io/ioutil"
	"log"
	"strconv"
	"strings"
)

type Tree struct {
	Left  *Tree
	Value int
	Right *Tree
}

func (tree *Tree) String() string {
	var v_s []string
	for v := range tree.DepthFirst() {
		v_s = append(v_s, strconv.FormatInt(int64(v), 10))
	}
	return strings.Join(v_s, " ")
}

func (tree *Tree) Hash(prev_hash int) int {
	if tree == nil {
		return prev_hash
	}
	hash := tree.Left.Hash(prev_hash)
	hash = (hash*(tree.Value+2) + (tree.Value + 2)) % 1000
	return tree.Right.Hash(hash)
}

//TODO: pre-alloc stack sizes?
func treesEqualSequential(tree_1 *Tree, tree_2 *Tree) bool {
	var tree_1_stack []*Tree
	var tree_2_stack []*Tree

	tree_1_it := tree_1
	tree_2_it := tree_2

	for (len(tree_1_stack) != 0 || len(tree_2_stack) != 0) ||
		(tree_1_it != nil || tree_2_it != nil) {

		//bear left
		for ; tree_1_it != nil; tree_1_it = tree_1_it.Left {
			tree_1_stack = append(tree_1_stack, tree_1_it)
			// println("1:", tree_1_it.String())
		}
		for ; tree_2_it != nil; tree_2_it = tree_2_it.Left {
			tree_2_stack = append(tree_2_stack, tree_2_it)
			// println("2:", tree_2_it.String())
		}

		//pop the stack
		tree_1_it = tree_1_stack[len(tree_1_stack)-1]
		tree_1_stack = tree_1_stack[:len(tree_1_stack)-1]
		tree_2_it = tree_2_stack[len(tree_2_stack)-1]
		tree_2_stack = tree_2_stack[:len(tree_2_stack)-1]

		// println("pop:", tree_1_it.Value, tree_2_it.Value)
		if tree_1_it.Value != tree_2_it.Value {
			return false
		}

		tree_1_it = tree_1_it.Right
		tree_2_it = tree_2_it.Right
	}

	// println("end:", len(tree_1_stack) != 0, len(tree_2_stack) != 0, tree_1_it != nil, tree_2_it != nil)
	// false if there are more nodes on one side
	if (len(tree_1_stack) != 0 || len(tree_2_stack) != 0) ||
		(tree_1_it != nil || tree_2_it != nil) {
		return false
	}

	return true
}

func (tree *Tree) depthFirst(c chan int) {
	if tree == nil {
		return
	}
	tree.Left.depthFirst(c)
	c <- tree.Value
	tree.Right.depthFirst(c)
}

func (tree *Tree) DepthFirst() <-chan int {
	//TODO: buffered?
	c := make(chan int)
	go func() {
		tree.depthFirst(c)
		close(c)
	}()
	return c
}

func insert(tree *Tree, number int) {
	if number < tree.Value {
		if tree.Left == nil {
			tree.Left = &Tree{nil, number, nil}
		} else {
			insert(tree.Left, number)
		}
	} else {
		if tree.Right == nil {
			tree.Right = &Tree{nil, number, nil}
		} else {
			insert(tree.Right, number)
		}
	}
	return
}

func ReadTrees(inputFile *string) []*Tree {
	var trees []*Tree
	content, err := ioutil.ReadFile(*inputFile)

	if err != nil {
		log.Fatal(err)
	}

	for _, row := range strings.Split(string(content), "\n") {
		if row == "" {
			break
		}
		var root *Tree
		for _, number_s := range strings.Split(string(row), " ") {
			number, err := strconv.ParseInt(number_s, 10, 32)
			if err != nil {
				log.Fatal(err)
			}
			if root == nil {
				root = &Tree{nil, int(number), nil}
			} else {
				insert(root, int(number))
			}
		}
		trees = append(trees, root)
	}
	return trees
}
