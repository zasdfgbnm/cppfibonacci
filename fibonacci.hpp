#ifndef _CPP_FIBONACCI_
#define _CPP_FIBONACCI_

/** \mainpage cppfibonacci: a C++ implementation of Fibonacci heap
 *
 * For API, see fibonacci_heap.
 *
 * For sample code, see @ref example.cpp.
 */

#include <functional>
#include <tuple>
#include <initializer_list>
#include <memory>

#ifdef FIBONACCI_HEAP_TEST_FRIEND
class FIBONACCI_HEAP_TEST_FRIEND;
#endif

/** \brief A C++ implementation of Fibonacci heap
 *
 * @param K the type for keys
 * @param T the type for data
 * @param Compare the class that define the order of keys, with default value the "<".
 */
template <typename K, typename T, typename Compare=std::less<K>>
class fibonacci_heap {

	/** To allow user defined test class to access private members of this class,
	  * simply define the test class name as macro FIBONACCI_HEAP_TEST_FRIEND
	  */
	#ifdef FIBONACCI_HEAP_TEST_FRIEND
	friend class FIBONACCI_HEAP_TEST_FRIEND;
	#endif

	class internal_data;

	/** \brief the internal class responsible for the structure in Fibonacci heap
	 * Structural information and data are stored to make it easier for std::shared_ptr
	 * to automatically clean up memory without destroying user's pointer to data.
	 */
	class internal_structure {
		bool childcut;
		size_t degree;
		std::shared_ptr<internal_data> data;
		std::shared_ptr<internal_structure> right_sibling;
		std::weak_ptr<internal_structure> left_sibling;
		std::shared_ptr<internal_structure> child;
		std::weak_ptr<internal_structure> parent;
		std::weak_ptr<fibonacci_heap> fh; // the Fibonacci heap this node is in
		~internal_structure() {
			data->structure = nullptr;
			// cut loops inside child's sibling list so that std::shared_ptr can
			// automatically free unneeded memory
			if(child) child->right_sibling = nullptr;
		}
	};

	/** \brief the interal class used to store data in Fibonacci heap */
	class internal_data {
	public:
		std::weak_ptr<internal_structure> structure;
		K key;
		T data;
		~internal_data() {
			if(!structure.expired()) throw "data node in use, destructing will cause unexpected behavior";
		}
	};

	/** \brief recursively duplicate nodes and create a new tree, including structure node and data node
	 *
	 * @param root the root node of the tree to be duplicated
	 *
	 * @param head used to denote what phase this function is doing. If head==nullptr,
	 * then this function has just started at a doubly linked list; if head!=nullptr,
	 * then this function is checking inside the doubly linked list and head is the first
	 * element of the doubly linked list.
	 *
	 * @param newhead used only in the case when head!=nullptr, then newhead is
	 * the pointer towards the duplicated node of head
	 *
	 * @return pointer to the node of the duplicated tree
	 */
	static std::shared_ptr<internal_structure> duplicate_nodes(std::weak_ptr<fibonacci_heap> fh, std::shared_ptr<const internal_structure> root,std::shared_ptr<const internal_structure> head,std::shared_ptr<internal_structure> newhead) {
		if(root==head) return nullptr;
		std::shared_ptr<internal_structure> newroot = std::make_shared<internal_structure>(*root);
		if(head==nullptr) {
			head = root;
			newhead = newroot;
		}
		// setup new data
		std::shared_ptr<internal_data> newroot_data = std::make_shared<internal_data>(root->data);
		newroot_data->structure = newroot;
		newroot->data = newroot_data;
		// setup new fh
		newroot->fh = fh;
		// setup new right_sibling
		newroot->right_sibling = duplicate_nodes(fh,root->right_sibling, head, newhead);
		if(newroot->right_sibling==nullptr)
			newroot->right_sibling = newhead;
		// setup new left_sibling
		newroot->right_sibling->left_sibling = newroot;
		// setup new child
		newroot->child = duplicate_nodes(fh,root->child, nullptr, nullptr);
		// setup new parent
		newroot->parent = nullptr;
		if(newroot->child) newroot->child->parent = newroot;
	}

	std::shared_ptr<internal_structure> min;
	size_t _size = 0;

public:

	/** \brief Create an empty Fibonacci heap. */
	fibonacci_heap() = default;

	/** \brief Initialize a Fibonacci heap from list of key data pairs.
	 * @param list the list of key data pairs
	 */
	fibonacci_heap(std::initializer_list<std::tuple<K,T>> list) {
		for(auto &i:list)
			insert(std::get<0>(i),std::get<1>(i));
	}

	/** \brief the copy constructor.
	 *
	 * Shallow copy will mess up the data structure and therefore is not allowed.
	 * Whenever the user tries to make a copy of a fibonacci_heap object, a deep
	 * copy will be made.
	 *
	 * Also note that the node object at old Fibonacci heap can not be used at
	 * copied Fibonacci heap.
	 *
	 * @param old the Fibonacci heap to be copied
	 */
	fibonacci_heap(const fibonacci_heap &old):_size(old._size),min(duplicate_nodes(*this,old.min,nullptr,nullptr)) {}

	~fibonacci_heap() {
		// cut loops inside the forest list so that std::shared_ptr can
		// automatically free unneeded memory
		if(min) min->right_sibling = nullptr;
	}

	/** \brief the copy assignment operator
	 *
	 * Shallow copy will mess up the data structure and therefore is not allowed.
	 * Whenever the user tries to a fibonacci_heap object to another object, a deep
	 * copy will be made.
	 *
	 * Also note that the node object at old Fibonacci heap can not be used at
	 * the new Fibonacci heap.
	 *
	 * @param old the Fibonacci heap to be copied
	 *
	 * @return reference to this object
	 */
	fibonacci_heap& operator = (const fibonacci_heap &old) {
		// clean up the current Fibonacci heap
		if(min!=nullptr) {
			~fibonacci_heap();
		}
		// make a copy of old
		_size = old._size;
		min = duplicate_nodes(*this,old.min, nullptr, nullptr);
		return *this;
	}

	/** \brief Reference to nodes in Fibonacci heap.
	 *
	 * Objects of node should be returned from methods of fibonacci_heap,
	 * and will keep valid throughout the whole lifetime of the Fibonacci heap
	 * no matter what operations the users did.
	 */
	class node {

		/** \brief pointer to interanl node */
		std::shared_ptr<internal_data> internal;

		/** \brief create a node object from internal nodes
		 *
		 * This is a private constructor, so the users are not allowed to create a node object.
		 * @param internal pointer to internal node
		 */
		node(std::shared_ptr<internal_structure> internal):internal(internal->data){}

	public:

		/** \brief get the key of this node.
		 * @return the key of this node
		 */
		K key() const { return internal->key; }

		/** \brief get the data stored in this node.
		 * @return the lvalue holding the data stored in this node
		 */
		T &data() { return internal->data; }

		/** \brief get the data stored in this node.
		 * @return the rvalue holding the data stored in this node
		 */
		const T &data() const { return internal->data; }

	};

	/** \brief Return the number of elements stored.
	 *
	 * @return number of elements stored in this Fibonacci heap
	 */
	size_t size() { return _size; }

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param data the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key,const T &data);

	/** \brief Insert an element.
	 *
	 * @param key the key of the element to be inserted
	 * @param value the data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(K key,const T &&data);

	/** \brief Insert an element.
	 *
	 * @param node the node object holding the key and data of the element to be inserted
	 * @return node object holding the inserted element
	 */
	node insert(node n) { return insert(n.key(),n.data()); }

	/** \brief Return the top element.
	 * @return the node object on the top
	 */
	node top() const { return node(min); }

	/** \brief Meld another Fibonacci heap to this Fibonacci heap.
	 * @param heap the Fibonacci heap to be melded
	 */
	void meld(const fibonacci_heap<K,T,Compare> &heap);

	/** \brief Descrease (or increase if you use greater as Compare) the key of the given node.
	 *
	 * @param node the node object holding the key and data of the element to be inserted
	 * @param new_key the new key of the node
	 */
	void decrease_key(node,K new_key);

	/** \brief Remove the top element.
	 * @return the removed node object
	 */
	node remove();

	/** \brief Remove the element specified by the node object.
	 * @param n the node to be removed
	 * @return the removed node object
	 */
	node remove(node n);
};

#endif