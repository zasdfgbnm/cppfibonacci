#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <functional>
#include <tuple>
#include <random>
#include <algorithm>
#include <limits>

using namespace std;

#define FIBONACCI_HEAP_TEST_FRIEND fibonacci_test
#include "fibonacci.hpp"

/** \brief container than contains implementation of tests */
class fibonacci_test {

	// this class is only a container, creating an object of this class is not allowed
	fibonacci_test();

public:

	// useful types
	using fh_t = fibonacci_heap<int,int>;
	using sn_t = typename fh_t::internal_structure;
	using dn_t = typename fh_t::internal_data;

private:

	/** \brief recursively run consistency test on each internal_structure node
	 *
	 * @param node the node to be tested
	 *
	 * @param parent the value of parent pointer that this node is supposed to have
	 *
	 * @param head used to denote what phase this function is doing. If head==nullptr,
	 * then this function has just started at a doubly linked list; if head!=nullptr,
	 * then this function is testing inside the doubly linked list and head is the first
	 * element of the doubly linked list.
	 *
	 * @return number of elements inside the doubly linked list tested starting at from
	 * this point including the parameter "node" itself
	 */
	static void _data_structure_consistency_test(shared_ptr<sn_t> node, shared_ptr<sn_t> parent, shared_ptr<sn_t> head, size_t &degree) {
		// if sibling test done
		if (node==head) {
			degree = 0;
			return;
		}
		// if this is the beginning of sibling test
		if (head==nullptr) head = node;
		// test min-tree property
		if(parent) ASSERT_LE(parent->data->key,node->data->key);
		// test parent and sibling pointers
		ASSERT_EQ(node->parent.lock(),parent);
		ASSERT_EQ(node->left_sibling.lock()->right_sibling,node);
		ASSERT_EQ(node->right_sibling->left_sibling.lock(),node);
		// test structure and data pointers
		ASSERT_EQ(node->data->structure.lock(),node);
		// recursively run test on child and test degree
		size_t calculated_degree;
		_data_structure_consistency_test(node->child, node, nullptr,calculated_degree);
		ASSERT_EQ(node->degree,calculated_degree);
		// recursively run test on siblings
		_data_structure_consistency_test(node->right_sibling, parent, head,degree);
		degree++;
	}

	/** \brief run binomial property test on a tree rooted at root
	 *
	 * @param root the root of the tree to be tested
	 */
	static void _expect_binomial(shared_ptr<sn_t> root) {
		size_t degree = root->degree;
		if(degree == 0) {
			ASSERT_EQ(root->child,nullptr);
			return;
		}
		bool children_degrees[degree];
		for(bool &i:children_degrees)
			i = false;
		shared_ptr<sn_t> p=root->child;
		do {
			_expect_binomial(p);
			ASSERT_FALSE(children_degrees[p->degree]);
			children_degrees[p->degree] = true;
			p=p->right_sibling;
		} while(p!=root->child);
		for(bool i:children_degrees)
			ASSERT_TRUE(i);
	}

	/** \brief test if the min pointer really point to the min */
	static void test_min_ptr(const fh_t &fh) {
		for(shared_ptr<sn_t> p=fh.min->right_sibling; p!=fh.min; p=p->right_sibling) {
			ASSERT_LE(fh.min->data->key,p->data->key);
		}
	}

	/** \brief count nodes in Fibonacci heap */
	static size_t count_nodes(shared_ptr<sn_t> root) {
		if(root==nullptr) return 0;
		size_t sum = 0;
		shared_ptr<sn_t> p = root;
		do {
			sum += 1 + count_nodes(p->child);
			p = p->right_sibling;
		} while(p!=root);
		return sum;
	}

	/** \brief test if element is in Fibonacci heap */
	static void element_in(shared_ptr<sn_t> e, const fh_t &fh) {
		while(!e->parent.expired())
			e = e->parent.lock();
		bool found = false;
		shared_ptr<sn_t> p = fh.min;
		do {
			if(p==e) {
				found = true;
				break;
			}
			p = p->right_sibling;
		} while(p!=fh.min);
		ASSERT_TRUE(found);
	}

	/** \brief test if all the forests given have the same structure */
	static void expect_same_tree_structure(vector<shared_ptr<sn_t>> nodes) {
		// if pointers are null, return
		bool allnull = true;
		bool anynull = false;
		for(shared_ptr<sn_t> &i:nodes) {
			allnull = allnull && (i==nullptr);
			anynull = anynull || (i==nullptr);
		}
		ASSERT_EQ(allnull,anynull);
		if(anynull) return;
		// traverse sibling list
		vector<shared_ptr<sn_t>> ps = nodes;
		bool done = false;
		do {
			// check if keys and values of different p are the same
			for(shared_ptr<sn_t> &p1:ps){
				for(shared_ptr<sn_t> &p2:ps){
					ASSERT_EQ(p1->data->key,p2->data->key);
					ASSERT_EQ(p1->data->data,p2->data->data);
				}
				vector<shared_ptr<sn_t>> children = nodes;
				for(shared_ptr<sn_t> &i:children) {
					i = i->child;
				}
				expect_same_tree_structure(children);
			}
			// update ps
			for(shared_ptr<sn_t> &p:ps)
				p = p->right_sibling;
			// see if we are done with this sibling list
			bool alldone = true;
			bool anydone = false;
			for(size_t i=0;i<nodes.size();i++) {
				alldone = alldone && (ps[i]==nodes[i]);
				anydone = anydone || (ps[i]==nodes[i]);
			}
			ASSERT_EQ(anydone,alldone);
			done = anydone;
		} while(!done);
	}

public:

	/** \brief test the consistency of the forest of min trees maintained inside fibonacci_heap
	*
	* The following things are tested:
	* 1. parent pointer
	* 2. sibling pointers
	* 3. degrees
	* 4. data and structure pointers
	* 5. min-tree property
	* 6. min pointer of Fibonacci heap
	* 7. size
	* 8. max_degree
	*/
	static void data_structure_consistency_test(const fh_t &fh) {
		// test for 1-5
		size_t unused;
		_data_structure_consistency_test(fh.min,nullptr,nullptr,unused);
		// test for 6
		test_min_ptr(fh);
		// test for 7
		ASSERT_EQ(fh._size,count_nodes(fh.min));
		// test for 8
		size_t max_deg = fh.max_degree();
		shared_ptr<sn_t> p = fh.min;
		do {
			ASSERT_LE(p->degree,max_deg);
			p = p->right_sibling;
		} while(p!=fh.min);
	}

	/** \brief test whether the fibonacci_heap object is copied/moved correctly
	 *
	 * The following things are tested:
	 * 1. Are the new data structure consistent?
	 * 2. Is the property that there is no overlap between old and new Fibonacci heap satisfied?
	 * 3. Are the tree structures kept the same?
	 */
	static void copy_move_test(const fh_t &fh) {
		fh_t fh2(fh); // copy constructor
		fh_t fh3 = fh; // copy constructor
		fh_t fh4;
		fh4 = fh; // assignment
		fh_t fh5 = fh_t(fh); // copy constructor, then move constructor
		fh_t fh6;
		fh6 = fh_t(fh); // copy constructor, then assignment
		// test for 1
		data_structure_consistency_test(fh);
		data_structure_consistency_test(fh2);
		data_structure_consistency_test(fh3);
		data_structure_consistency_test(fh4);
		data_structure_consistency_test(fh5);
		data_structure_consistency_test(fh6);
		// test for 2
		ASSERT_NE(fh.min, fh2.min);
		ASSERT_NE(fh.min, fh3.min);
		ASSERT_NE(fh.min, fh4.min);
		ASSERT_NE(fh.min, fh5.min);
		ASSERT_NE(fh.min, fh6.min);
		// test for 3
		expect_same_tree_structure({fh.min,fh2.min,fh3.min,fh4.min,fh5.min,fh6.min});
	}

	/** \brief expect that this fibonacci_heap must be a binomial heap
	 *
	 * In the case that only insert, meld and remove min is performed,
	 * a Fibonacci heap should be exactly the same as a binomial heap.
	 * This method is designed to be called in this case to expect that
	 * the Fibonacci heap is actually a binomial heap.
	 */
	static void expect_binomial(const fh_t &fh) {
		shared_ptr<sn_t> p=fh.min;
		do {
			_expect_binomial(p);
			p=p->right_sibling;
		} while(p!=fh.min);
	}

	/** \brief test whether the cleanup procedure of a Fibonacci heap works well during descruction
	 *
	 * The following things are tested:
	 * 1. Are all the structure nodes destroyed?
	 * 2. Are all the data nodes without external reference destroyed?
	 * 3. Are the reference counts of all the data nodes with external reference decrease by one?
	 *
	 * @param fhptr the pointer pointing to the Fibonacci heap to be destroyed
	 */
	void destroy_and_test(shared_ptr<fh_t> &fhptr) {
		vector<weak_ptr<sn_t>> sn_clean_list;
		vector<weak_ptr<dn_t>> dn_clean_list;
		vector<tuple<weak_ptr<dn_t>,size_t>> dn_keep_list;
		// dump out weak pointers to all the structure and data nodes
		function<void (shared_ptr<sn_t>,shared_ptr<sn_t>)> traverse = [&](shared_ptr<sn_t> node,shared_ptr<sn_t> head) {
			if(node==head) return;
			if(head==nullptr) head=node;
			sn_clean_list.push_back(node);
			if(node->data.unique())
				dn_clean_list.push_back(node->data);
			else
				dn_keep_list.push_back(make_tuple(node->data,node->data.use_count()));
			traverse(node->right_sibling,head);
			traverse(node->child,nullptr);
		};
		traverse(fhptr->min,nullptr);
		// destroy
		ASSERT_TRUE(fhptr.unique());
		fhptr.reset();
		// test for 1
		for(weak_ptr<sn_t> &i : sn_clean_list) {
			ASSERT_TRUE(i.expired());
		}
		// test for 2
		for(weak_ptr<dn_t> &i : dn_clean_list) {
			ASSERT_TRUE(i.expired());
		}
		// test for 3
		for(tuple<weak_ptr<dn_t>,size_t> &i : dn_keep_list) {
			ASSERT_EQ(get<0>(i).use_count(),get<1>(i)-1);
			ASSERT_TRUE(get<0>(i).lock()->structure.expired());
		}
	}
};

/** \brief an engine to do random operations and generate random Fibonacci heaps */
class random_engine {
public:
	using fh_t = fibonacci_heap<int,int>;
	random_device r;
	default_random_engine rng;
	uniform_real_distribution<double> u01 = uniform_real_distribution<double>(0,1);
	uniform_int_distribution<int> ui01 = uniform_int_distribution<int>(0,1);
	uniform_int_distribution<int> uint = uniform_int_distribution<int>(numeric_limits<int>::min(),numeric_limits<int>::max());
	shared_ptr<fh_t> fh[2];
	vector<fh_t::node> nodes[2];

	random_engine():rng(r()) {}

	double pnew = 0.1;
	double pcopy = 0.5;
	double pdestroy = 0.005;
	double pmeld = 0.1;
	double premoveany = 0.5;
	double pdecreasekey = 0.5;

	/** \brief the probability that a Fibonacci heap has a given size */
	virtual double probability(size_t size) {
		double mu = 500;
		double s = 200;
		return exp(-(size-mu)*(size-mu)/(2*s*s));
	};

	/** \brief make a random step */
	virtual void random_step() {
		cout << "size = ";
		if(fh[0]==nullptr)
			cout << "null";
		else
			cout << fh[0]->size();
		cout << " , ";
		if(fh[1]==nullptr)
			cout << "null";
		else
			cout << fh[1]->size();
		cout << endl;
		// initialize or nothing
		if(fh[0]==nullptr&&fh[1]==nullptr) {
			cout << "initialize" << endl;
			int i = ui01(rng);
			fh[i] = make_shared<fh_t>();
			return;
		}
		// meld or nothing
		if(fh[0]!=nullptr && fh[1]!=nullptr && u01(rng)<pmeld) {
			cout << "meld" << endl;
			int i = ui01(rng);
			fh[i]->meld(*fh[1-i]);
			nodes[i].insert(nodes[i].end(),nodes[1-i].begin(),nodes[1-i].end());
			nodes[1-i].clear();
			return;
		}
		// destroy one or nothing
		if(fh[0]!=nullptr && fh[1]!=nullptr && u01(rng)<pdestroy) {
			cout << "destroy" << endl;
			int i = ui01(rng);
			fh[i].reset();
			nodes[i].clear();
			return;
		}
		// create new or nothing
		int i = ui01(rng);
		if(fh[i]==nullptr && u01(rng)<pnew) {
			cout << "create new" << endl;
			if(u01(rng)<pcopy)
				fh[i] = make_shared<fh_t>(*fh[1-i]);
			else
				fh[i] = make_shared<fh_t>();
			return;
		}
		if(fh[i]==nullptr)
			i = 1-i;
		// insert, remove, remove(node), or decrease_key
		double movetype = u01(rng);
		if(fh[i]->size()==0)
			movetype = 0;
		double acceptrate = 0;
		if(movetype<0.5) // insert
			acceptrate = probability(fh[i]->size()+1)/probability(fh[i]->size());
		else
			acceptrate = probability(fh[i]->size()-1)/probability(fh[i]->size());
		cout << "accept rate = " << acceptrate << endl;
		if(u01(rng)<acceptrate) {
			if(movetype<0.5) {
				cout << "insert" << endl;
				// insert
				nodes[i].push_back(fh[i]->insert(uint(rng),uint(rng)));
				return;
			} else {
				if((!nodes[i].empty())&&u01(rng)<premoveany) {
					cout << "remove(node)" << endl;
					// remove(node)
					size_t s = nodes[i].size();
					uniform_int_distribution<int> dist(0,s-1);
					size_t rmpos = dist(rng);
					cout << "rmpos = " << rmpos << endl;
					fh[i]->remove(nodes[i][rmpos]);
					cout << "done remove" << endl;
					nodes[i].erase(nodes[i].begin()+rmpos);
					return;
				} else { // remove()
					cout << "remove" << endl;
					fh_t::node r = fh[i]->remove();
					nodes[i].erase(remove(nodes[i].begin(),nodes[i].end(),r),nodes[i].end());
					return;
				}
			}
		} else {
			if((!nodes[i].empty())&&u01(rng)<pdecreasekey) {
				cout << "decrease key" << endl;
				// decrease_key(node)
				size_t s = nodes[i].size();
				uniform_int_distribution<int> dist(0,s-1);
				fh_t::node n = nodes[i][dist(rng)];
				uniform_int_distribution<int> dist2(numeric_limits<int>::min(),n.key());
				fh[i]->decrease_key(n,dist2(rng));
				return;
			} else {
				cout << "insert & remove min" << endl;
				// insert then remove min
				nodes[i].push_back(fh[i]->insert(uint(rng),uint(rng)));
				fh_t::node r = fh[i]->remove();
				nodes[i].erase(remove(nodes[i].begin(),nodes[i].end(),r),nodes[i].end());
				return;
			}
		}
	}
};

/** \brief run random operations and check consistency after each operation */
TEST(whitebox,consistency) {
	int steps = 10000;
	random_engine r;
	for(int i=0;i<steps;i++) {
		cout << "step = " << i << endl;
		r.random_step();
		// int a01[] = {0,1};
		// for(int i:a01) {
		// 	if(r.fh[i]) fibonacci_test::data_structure_consistency_test(*r.fh[i]);
		// }
	}
}

/** \brief randomly insert,remove min, meld elements and check if binomial heap
 * properties are maintained after each operation */
TEST(whitebox,binomial) {

}

/** \brief generate a random Fibonacci heap and test copy and move constructor and assignment operator */
TEST(whitebox,copy_move) {

}

/** \brief generate a random Fibonacci heap and test if this heap destroy correctly */
TEST(whitebox,destroy) {}

/** \brief randomly insert, remove, change or merge some elements and see if
 * Fibonacci heap can generate a sorted list of remaining elements*/
TEST(blackbox,sort) {}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
