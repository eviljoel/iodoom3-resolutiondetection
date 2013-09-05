// Copyright Adrian McMenamin 2010
// Copyright Joel Luellwitz 2013
// Licensed under the GNU GPL version 3
// or any later version at your discretion

#ifndef NULL
#define NULL 0
#endif

//template <typename KEY, typename VALUE> class idBTree;

template <typename KEY, typename VALUE> class idBTreeNode {
	//friend class idBTree<KEY, VALUE>;

	public:
		KEY key;
		VALUE value;

	private:
		int color;
		idBTreeNode* up;
		idBTreeNode* left;
		idBTreeNode* right;
		idBTreeNode(const KEY& k, const VALUE& v);
		idBTreeNode(idBTreeNode* node);
		idBTreeNode(idBTreeNode& node);
		idBTreeNode* grandparent() const;
 		idBTreeNode* uncle() const;
		idBTreeNode* sibling() const;
		bool bothchildrenblack() const;
		bool equals(idBTreeNode*) const;
		bool lessthan(idBTreeNode*) const;
		void assign(idBTreeNode*);
};

template <typename KEY, typename VALUE> class idBTree {
	private:
		idBTreeNode<KEY, VALUE>* root;
		void balanceinsert(idBTreeNode<KEY, VALUE>*);
		void rotate3(idBTreeNode<KEY, VALUE>*);
		void rotate2(idBTreeNode<KEY, VALUE>*);
		void rotate2a(idBTreeNode<KEY, VALUE>*);
		void rotate1(idBTreeNode<KEY, VALUE>*);
		void transform2(idBTreeNode<KEY, VALUE>*);
		void free(idBTreeNode<KEY, VALUE>*);
		idBTreeNode<KEY, VALUE>* maxleft(idBTreeNode<KEY, VALUE>*) const;
		idBTreeNode<KEY, VALUE>* minright(idBTreeNode<KEY, VALUE>*) const;
		idBTreeNode<KEY, VALUE>* locatenode(idBTreeNode<KEY, VALUE>*, idBTreeNode<KEY, VALUE>*) const;
		void countup(idBTreeNode<KEY, VALUE>*, int&) const;
	public:
		void insertnode(idBTreeNode<KEY, VALUE>*, idBTreeNode<KEY, VALUE>*);
		bool removenode(idBTreeNode<KEY, VALUE>&);
		bool find(idBTreeNode<KEY, VALUE>&) const;
		idBTreeNode<KEY, VALUE>* min() const;
		idBTreeNode<KEY, VALUE>* max() const;
		int Size() const;
		idBTree();
		~idBTree();
};

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>::idBTreeNode(const KEY& k, const VALUE& v) {
	color = 1; //red
	key = k;
	value = v;
	up = NULL;
	left = NULL;
	right = NULL;
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>::idBTreeNode(idBTreeNode* node) {
	color = node->color;
	key = node->key;
	value = node->value;
	up = NULL;
	left = NULL;
	right = NULL;
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>::idBTreeNode(idBTreeNode& node) {
	color = node.color;
	key = node.key;
	value = node.value;
	up = NULL;
	left = NULL;
	right = NULL;
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>* idBTreeNode<KEY, VALUE>::sibling() const {
	idBTreeNode* sibling = NULL;
	if (up) {
		if (up->left == this) {
			sibling = up->right;
		} else {
			sibling = up->left;
		}
	}
	return sibling;
}


template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>* idBTreeNode<KEY, VALUE>::grandparent() const {
	idBTreeNode* grandparent = NULL;
	if (up) {
		grandparent = up->up;
	}
	return grandparent;
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>* idBTreeNode<KEY, VALUE>::uncle() const {
	idBTreeNode* g = grandparent();
	idBTreeNode* uncle = NULL;
	if (g) {
		if (g->left == up) {
			uncle = g->right;
		} else {
			uncle = g->left;
		}
	}
	return uncle;
}

template <typename KEY, typename VALUE> bool idBTreeNode<KEY, VALUE>::bothchildrenblack() const {
	bool bothChildrenBlack = true;
	if (right && right->color == 1) {
		bothChildrenBlack = false;
	} else if (left && left->color == 1) {
		bothChildrenBlack = false;
	}
	return bothChildrenBlack;
}


template <typename KEY, typename VALUE> bool idBTreeNode<KEY, VALUE>::equals(idBTreeNode* rbn) const {
	return key == rbn->key;
}

template <typename KEY, typename VALUE> bool idBTreeNode<KEY, VALUE>::lessthan(idBTreeNode* rbn) const {
	return key < rbn->key;
}

template <typename KEY, typename VALUE> void idBTreeNode<KEY, VALUE>::assign(idBTreeNode<KEY, VALUE>* v) {
	key = v->key;
	value = v->value;
}

idBTree::~idBTree() {
	free(root);
}

template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::free(idBTreeNode<KEY, VALUE>* v) {
	if (v != NULL) {
		free(v->left);
		free(v->right);
		delete v;
	}
}

idBTree::idBTree() {
	root = NULL;
}

// turn line of two reds and a black into black with two children
template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::rotate2(idBTreeNode<KEY, VALUE>* node) {
	if (node && node->up) {
		idBTreeNode<KEY, VALUE>* gp = node->grandparent();
		idBTreeNode<KEY, VALUE>* par = NULL;

		idBTreeNode<KEY, VALUE>* centrenode = node->up;
		if (gp) {
			par = gp->up;
			if (par) {
				if (par->left == gp) {
					par->left = centrenode;
				} else {
					par->right = centrenode;
				}
			}
		}

		if (node->up->right == node) {
			idBTreeNode<KEY, VALUE>* centreleft = centrenode->left;
			centrenode->color = 0;
			centrenode->left = gp;
			if (gp) {
				gp->up = centrenode;
				gp->color = 1;
				gp->right = centreleft;
				if (centreleft) {
					centreleft->up = gp;
				}
			}
		} else {
			idBTreeNode<KEY, VALUE>* centreright = centrenode->right;
			centrenode->color = 0;
			centrenode->right = gp;
			if (gp) {
				gp->up = centrenode;
				gp->color = 1;
				gp->left = centreright;
				if (centreright) {
					centreright->up = gp;
				}
			}
		}
		centrenode->up = par;
		if (!par) {
			root = centrenode;
		}
	}
}

template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::rotate3(idBTreeNode<KEY, VALUE>* node) {
	if (node && node->up) {
		idBTreeNode<KEY, VALUE>* par = node->up;
		idBTreeNode<KEY, VALUE>* righty = node->right;
		idBTreeNode<KEY, VALUE>* lefty = node->left;

		if (par->left == node) {
			par->left = righty;
			righty->color = 0;
			righty->up = par;
			node->up = righty;
			node->right = righty->left;
			righty->left = node;
			node->color = 1;
		} else {
			par->right = lefty;
			lefty->color = 0;
			lefty->up = par;
			node->up = lefty;
			node->left = lefty->right;
			lefty->right = node;
			node->color = 1;
		}
	}
}			

template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::rotate2a(idBTreeNode<KEY, VALUE>* node) {
	if (node && node->up) {
		idBTreeNode<KEY, VALUE>* par = node->up;
		idBTreeNode<KEY, VALUE>* gp = node->grandparent();
		idBTreeNode<KEY, VALUE>* righty = node->right;
		idBTreeNode<KEY, VALUE>* lefty = node->left;
		if (gp) {
			if (gp->left == par) {
				gp->left = node;
			} else {
				gp->right = node;
			}
			node->up = gp;
		} else {
			root = node;
			node->up = NULL;
		}

		if (par->right == node) {
			node->left = par;
			par->up = node;
			par->right = lefty;
			if (lefty) {
				lefty->up = par;
			}

		} else {
			node->right = par;
			par->up = node;
			par->left = righty;
			if (righty) {
				righty->up = par;
			}
		}
		par->color = 1;
		node->color = 0;
	}
}	

//straighten zig zag of two reds
template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::rotate1(idBTreeNode<KEY, VALUE>* node) {
	if (node) {
		idBTreeNode<KEY, VALUE>* par = node->up;
		idBTreeNode<KEY, VALUE>* rightnode = node->right;
		idBTreeNode<KEY, VALUE>* leftnode = node->left;
		idBTreeNode<KEY, VALUE>* rightleft = NULL;
		idBTreeNode<KEY, VALUE>* leftright = NULL;

		if (par) {
			if (par->left == node) {
				par->left = rightnode;
				if (rightnode) {
					rightleft = rightnode->left;
					rightnode->up = par;
					rightnode->left = node;
				}
				node->right = rightleft;
				if (rightleft) {
					rightleft->up = node;
				}
				node->up = rightnode;
			} else {
				par->right = leftnode;
				if (leftnode) {
					leftright = leftnode->right;
					leftnode->up = par;
					leftnode->right = node;
				}
				node->left = leftright;
				if (leftright) {
					leftright->up = node;
				}
				node->up = leftnode;
			}
		}
	}
}

template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::transform2(idBTreeNode<KEY, VALUE>* node) {
	int oldcolor = node->up->color;
	rotate2a(node);
	node->color = oldcolor;
	if (node->left) {
		node->left->color = 0;
	}
	if (node->right) {
		node->right->color = 0;
	}
}

template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::balanceinsert(idBTreeNode<KEY, VALUE>* node) {
	if (node->up) {
		if (node->up->color != 0) {
			if (node->uncle() && node->uncle()->color == 1) {
				node->up->color = 0;
				node->uncle()->color = 0;
				node->grandparent()->color = 1;
				balanceinsert(node->grandparent());
			} else {

				if (node->grandparent()->left == node->up) {
					if (node->up->right == node) {
						rotate1(node->up);
						node = node->left;
					}
					rotate2(node);
				} else {
					if (node->up->left == node) {
						rotate1(node->up);
						node = node->right;
					}
					rotate2(node);
				}
			}
		}
	} else {
		node->color = 0;
	}
}

template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::insertnode(idBTreeNode<KEY, VALUE>* insert, idBTreeNode<KEY, VALUE>* node) {
	if (node == NULL) {
		root = insert;
		root->color = 0;
	} else if (insert->lessthan(node)) {
		if (node->left == NULL) {
			node->left = insert;
			node->left->up = node;
			node = node->left;
			balanceinsert(node);
		} else {
			insertnode(insert, node->left);
		}
	} else {
		if (node->right == NULL) {
			node->right = insert;
			node->right->up = node;
			node = node->right;
			balanceinsert(node);
		} else {
			insertnode(insert, node->right);
		}
	}
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>* idBTree<KEY, VALUE>::locatenode(idBTreeNode<KEY, VALUE>* v, idBTreeNode<KEY, VALUE>* node) const {
	idBTreeNode* locatedNode = NULL;
	if (node != NULL) {
		if (v->equals(node)) {
			locatedNode = node;
		} else if (v->lessthan(node)) {
			locatedNode = locatenode(v, node->left);
		} else {
			locatedNode = locatenode(v, node->right);
		}
	}
	return locatedNode;
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>* idBTree<KEY, VALUE>::minright(idBTreeNode<KEY, VALUE>* node) const {
	idBTreeNode<KEY, VALUE>* minRight;
	if (node->left) {
		minRight = minright(node->left);
	} else {
		minRight = node;
	}
	return minRight;
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>* idBTree<KEY, VALUE>::min() const {
	idBTreeNode<KEY, VALUE>* min = NULL;
	if (root) {
		min = root;
		while(min->left) {
			min = min->left;
		}
	}
	return min;
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>* idBTree<KEY, VALUE>::max() const {
	idBTreeNode<KEY, VALUE>* max = NULL;
	if (root) {
		max = root;
		while(max->right) {
			max = max->right;
		}
	}
	return max;
}

template <typename KEY, typename VALUE> void idBTree<KEY, VALUE>::countup(idBTreeNode<KEY, VALUE>* node, int& x) const {
	if (node != NULL) {
		countup(node->left, x);
		countup(node->right, x);
		x++;
	}
}

template <typename KEY, typename VALUE> int idBTree<KEY, VALUE>::Size() const {
	int cnt = 0;
	countup(root, cnt);
	return cnt;
}

template <typename KEY, typename VALUE> idBTreeNode<KEY, VALUE>* idBTree<KEY, VALUE>::maxleft(idBTreeNode<KEY, VALUE>* node) const {
	idBTreeNode* maxLeft;
	if (node->right) {
		maxLeft = maxleft(node->right);
	} else {
		maxLeft = node;
	}
	return maxLeft;
}

template <typename KEY, typename VALUE> bool idBTree<KEY, VALUE>::find(idBTreeNode<KEY, VALUE>& v) const {
	idBTreeNode* located = locatenode(&v, root);
	return located != NULL;
}

template <typename KEY, typename VALUE> bool idBTree<KEY, VALUE>::removenode(idBTreeNode<KEY, VALUE>& v) {
	idBTreeNode<KEY, VALUE>* located = locatenode(&v, root);
	if (located != NULL) {

		idBTreeNode<KEY, VALUE>* lefty = located->left;
		idBTreeNode<KEY, VALUE>* righty =  located->right;
		if (lefty && righty){
			idBTreeNode<KEY, VALUE>* altnode = maxleft(located->left);
			if (altnode->color == 0) {
				altnode = minright(located->right);
			}
			located->assign(altnode);
			located = altnode;
			lefty = located->left;
			righty = located->right;
		}
	
		//located is now a node with only one child at most
		idBTreeNode<KEY, VALUE>* par = located->up;
		idBTreeNode<KEY, VALUE>* sibling = located->sibling();
		idBTreeNode<KEY, VALUE>* follow = NULL;
		if (lefty) {
			follow = lefty;
		} else {
			follow = righty;
		}

		if (par) {
			if (par->left == located) {
				par->left = follow;
			} else {
				par->right = follow;
			}
		} else {
			root = follow;
		}

		if (follow) {
			follow->up = par;
		}

		//easy to remove a red
		if (located->color == 1) {
			delete located;
			return true;
		}

		//also easy if follow is red
		if (follow && follow->color == 1) {
			follow->color = 0;
			delete located;
			return true;
		}
	
		if (!follow && !par) {
			//tree is now empty
			delete located;
			root = NULL;
			return true;
		}

		//loop through the fixes
		do {
			if (sibling == root) {
				delete located;
				return true;
			}
			//test sibling status
			if (sibling) {
				//red?
				if (sibling->color == 1) {
					bool leftist = (par->left == sibling);
					rotate2a(sibling);
					sibling->color = 0;
					par->color = 1;
					if (follow) {
						sibling = follow->sibling();
					} else {
						if (leftist) {
							sibling = par->left;
						} else {
							sibling = par->right;
						}
					}
				}
				// case above can fall directly into case below
				if (par->color == 1) {
					if (sibling->bothchildrenblack()) {
						sibling->color = 1;
						par->color = 0;
						delete located;
						return true;
					}
				}
				else if (sibling->bothchildrenblack()) {
					sibling->color = 1;
					follow = par;
					sibling = follow->sibling();
					par = follow->up;
					if (par == NULL) {
						//at root can go no further
						delete located;
						return true;
					}
					continue;
				}
				if (par->right == sibling) {
					if (sibling->left && sibling->left->color == 1
						&& (sibling->right == NULL || (sibling->right &&
						sibling->right->color == 0))) {

						rotate3(sibling);
						sibling = sibling->up;
						continue;
					} else {
						transform2(sibling);
						delete located;
						return true;
					}
				} else if (par->left == sibling) {
					if (sibling->right && sibling->right->color == 1 &&
						(sibling->left == NULL || (sibling->left &&
						sibling->left->color == 0))) {

						rotate3(sibling);
						sibling = sibling->up;
						continue;
					} else {
						transform2(sibling);
						delete located;
						return true;
					}
				}
			} else {
				if (par->color == 1) {
					par->color = 0;
					delete located;
					return true;
				}
				else {
					if (!follow) {
						follow = located->up;
					} else {
						follow = follow->up;
					}
					par = follow->up;
				}
			}
			if (follow) {
				sibling = follow->sibling();
			}

		} while(true);
	}
}
