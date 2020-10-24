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

func (tree *Tree) Hash() (hash int) {
	hash = 1
	for v := range tree.DepthFirst() {
		hash = (hash*(v+2) + (v + 2)) % 1000
	}
	return
}

func (tree *Tree) depthFirstFill(values *[]int) {
	for v := range tree.DepthFirst() {
		*values = append(*values, v)
	}
	return
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
	c := make(chan int)
	go func() {
		tree.depthFirst(c)
		close(c)
	}()
	return c
}

func insert(tree *Tree, number int) *Tree {
	if tree == nil {
		return &Tree{nil, number, nil}
	}
	if number < tree.Value {
		tree.Left = insert(tree.Left, number)
	} else {
		tree.Right = insert(tree.Right, number)
	}
	return tree
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
			root = insert(root, int(number))
		}
		trees = append(trees, root)
	}
	return trees
}
