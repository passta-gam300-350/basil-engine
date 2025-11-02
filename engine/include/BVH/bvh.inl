/**
 * @file
 *  bvh.inl
 * @author
 *  Jia Zen Cheong, jiazen.c@digipen.edu
 * @date
 *  2025/06/10
 * @brief
 *  This file implemented the defintiion of struct
 *  and function needed to create a BVH using topdown
 *  and insertion (incremental) approach
 * @copyright
 *  Copyright (C) 2025 DigiPen Institute of Technology.
 */
#ifndef BVH_INL
#define BVH_INL

#include "bvh.h"
#include "bvh_logging.h"
#include "shapes.h"
#include <array>
#include <iomanip>
#include <cstring>
#include <limits>
#include <type_traits>
#include <stack>
#include <cmath>

 /**
 * @brief
 *  this function add object into the object linked list
 * @param T object
 *  the object to be added
 * @return
 *  void
 */
template<typename T>
void Bvh<T>::Node::AddObject(T object)
{
	if (firstObject == nullptr) // no object in node
	{
		// set first and last since it is the first one
		firstObject = object;
		lastObject = object;
		// there should be nothing in front and behind the first object
		object->bvhInfo.next = nullptr;
		object->bvhInfo.prev = nullptr;
	}
	else
	{
		lastObject->bvhInfo.next = object; // make the lastobject next point to the object, so it became second last
		object->bvhInfo.prev = lastObject; // now the new added is the new last object
		object->bvhInfo.next = nullptr; // last object should have no next object
		lastObject = object; // now the last object will become the new object
	}
	object->bvhInfo.node = this; // make the new object know this is his node
}

/**
* @brief
*  this function will calculate and return the depth
*  of a bvh tree
* @return int
*  the depth of the bvh
*/
template<typename T>
int Bvh<T>::Node::Depth() const
{
	if (this->IsLeaf())
	{
		return 0;
	}
	int depthLeft = children[0] ? children[0]->Depth() : -1; // use -1 to differentiate no nodes or is a leaf
	int depthRight = children[1] ? children[1]->Depth() : -1;
	return std::max(depthLeft, depthRight) + 1;
}

/**
* @brief
*  this function will calculate and return the size
*  of a bvh tree node
* @return
*  the size of a bvh tree node
*/
template<typename T>
int Bvh<T>::Node::Size() const
{
	int count = 1;
	if (this->IsLeaf() == false)
	{
		if (children[0] != nullptr)
		{
			count += children[0]->Size();
		}
		if (children[1] != nullptr)
		{
			count += children[1]->Size();
		}
	}
	return count;
}

/**
* @brief
*  this function will check if a node is a left,
*  means no children
* @return bool
*  true if is a leaf, else false
*/
template<typename T>
bool Bvh<T>::Node::IsLeaf() const
{
	if (children[0] == nullptr && children[1] == nullptr)
	{
		return true;
	}
	return false;
}

/**
* @brief
*  this function will return the object count
* @return unsigned
*  the count of the object
*/
template<typename T>
unsigned Bvh<T>::Node::ObjectCount() const
{
	unsigned int count = 0;
	T objectList = firstObject; // start from first object then loop
	while (objectList != nullptr)
	{
		count++;
		objectList = objectList->bvhInfo.next;
	}
	return count;
}

/**
* @brief
*  This is a debug function will
*  traverses the tree node and call the
*  function input by the user
* @param Fn func
*  the function to use
*/
template<typename T>
template<typename Fn>
void Bvh<T>::Node::TraverseLevelOrder(Fn func) const
{
	// FIFO is better because is easier to make it to traverse in level order, you store in two children, process it, then store another two children in queue, make it level order
	std::queue<Node const*> nodeLevelQueue;
	nodeLevelQueue.push(this);
	while (nodeLevelQueue.empty() == false)
	{
		Node const* currentProcessNode = nodeLevelQueue.front();
		nodeLevelQueue.pop();
		func(currentProcessNode); // use the lambda that is defined from the user
		if (currentProcessNode->IsLeaf() == false)
		{
			if (currentProcessNode->children[0] != nullptr) // children exist
			{
				nodeLevelQueue.push(currentProcessNode->children[0]);
			}
			if (currentProcessNode->children[1] != nullptr) // children exist
			{
				nodeLevelQueue.push(currentProcessNode->children[1]);
			}
		}
	}
}

/**
* @brief
*  This is a debug function will
*  traverses the tree objects and call the
*  function input by the user
* @param Fn func
*  the function to use
*/
template<typename T>
template<typename Fn>
void Bvh<T>::Node::TraverseLevelOrderObjects(Fn func) const
{
	std::queue<Node const*> nodeLevelQueue;
	nodeLevelQueue.push(this);
	while (nodeLevelQueue.empty() == false)
	{
		Node const* currentProcessNode = nodeLevelQueue.front();
		nodeLevelQueue.pop();
		T currentProcessObject = currentProcessNode->firstObject;
		while (currentProcessObject != nullptr)
		{
			func(currentProcessObject);
			currentProcessObject = currentProcessObject->bvhInfo.next;
		}
		if (currentProcessNode->IsLeaf() == false)
		{
			if (currentProcessNode->children[0] != nullptr) // children exist
			{
				nodeLevelQueue.push(currentProcessNode->children[0]);
			}
			if (currentProcessNode->children[1] != nullptr) // children exist
			{
				nodeLevelQueue.push(currentProcessNode->children[1]);
			}
		}
	}
}

/**
* @brief
*  This is a default ctor for Bvh
*/
template<typename T>
Bvh<T>::Bvh() : mRoot(nullptr), mObjectCount(0)
{

}

/**
* @brief
*  This is a destructor for Bvh
*/
template<typename T>
Bvh<T>::~Bvh()
{
	Clear(); // call clear to clear all the tree
}

/**
* @brief
*  This is a getter for Bvh root
* @return Bvh<T>::Node const*
*  the root
*/
template<typename T>
typename Bvh<T>::Node const* Bvh<T>::root() const
{
	return mRoot;
}

/**
* @brief
*  This is a function to check if a bvh is empty
* @return bool
*  if empty, true, else false
*/
template<typename T>
bool Bvh<T>::Empty() const
{
	if (mRoot)
	{
		return false;
	}
	return true;
}

/**
* @brief
*  This is a function to calculate and
*  return the depth of a whole bvh
* @return int
*  the depth of whole bvh
*/
template<typename T>
int Bvh<T>::Depth() const
{
	return mRoot ? mRoot->Depth() : -1;
}

/**
* @brief
*  This is a function to calculate and
*  return the size (amount of node) of a whole bvh
* @return int
*  the size (amount of node) of a whole bvh
*/
template<typename T>
int Bvh<T>::Size() const
{
	return mRoot ? mRoot->Size() : 0;
}

/**
* @brief
*  This is a helper clear function to clear all
*  the node
* @param Node* node
*  the node to start clear from
* @return void
*/
template<typename T>
void Bvh<T>::helper_clear(Node* node)
{
	if (node == nullptr)
	{
		return;
	}
	if (node->IsLeaf() == false)
	{
		helper_clear(node->children[0]);
		helper_clear(node->children[1]);
	}
	T clearObjectBvhInfo = node->firstObject;
	// this is just clear the object's bvh info, not the object itself
	while (clearObjectBvhInfo != nullptr)
	{
		T nextObjectToClear = clearObjectBvhInfo->bvhInfo.next;
		clearObjectBvhInfo->bvhInfo.next = nullptr;
		clearObjectBvhInfo->bvhInfo.prev = nullptr;
		clearObjectBvhInfo->bvhInfo.node = nullptr;
		clearObjectBvhInfo = nextObjectToClear;
	}
	delete node; // clear the bvh node itself
}

/**
* @brief
*  This is a clear function that utilise
*  the helper to clear the whole tree
* @return void
*/
template<typename T>
void Bvh<T>::Clear()
{
	if (mRoot != nullptr)
	{
		helper_clear(mRoot);
		mRoot = nullptr;
		mObjectCount = 0;
	}
}

/**
* @brief
*  This is a query function to
*  check which objects is in the
*  frustum
* @param Frustum const& frustum
*  the frustum to check with
* @return std::vector<unsigned>
*  the list of visible objects' id
*/
template<typename T>
std::vector<unsigned> Bvh<T>::Query(Frustum const& frustum) const
{
	std::vector<unsigned> resultList; // this is a list of visible object IDs
	if (mRoot == nullptr)
	{
		return resultList; // early return as there is a empty tree
	}
	std::stack<Node const*> nodeStack; // stack for depth first traversal, not using recursive
	nodeStack.push(mRoot);
	while (nodeStack.empty() == false) // main loop to visit every node
	{
		Node const* eachNode = nodeStack.top(); // get last node pushed
		nodeStack.pop();
		SideResult check = frustum.classify(eachNode->bv); // check current node with frustum
		if (check == eOUTSIDE) // if this node is outside, the child (subtree) is outside
		{
			continue;
		}
		if (check == eINSIDE) // if this node is completely inside, it child (subtree) should also be all inside, just grab all the objects
		{
			eachNode->TraverseLevelOrderObjects
			([&](auto const& object)
				{
					resultList.push_back(object->id);
				});
			continue;
		}
		if (check == eINTERSECTING) // partial overlap, traverse down the node
		{
			if (eachNode->IsLeaf()) // check if we reach leaf, if we leaf, means all the objects inside is intersecting
			{
				T object = eachNode->firstObject;
				while (object != nullptr)
				{
					resultList.push_back(object->id);
					object = object->bvhInfo.next;
				}
			}
			else // not leaf, we still need to check which children are visible, push to the stack to continue check
			{
				// LIFO, we check right subtree first
				if (eachNode->children[0] != nullptr)
				{
					nodeStack.push(eachNode->children[0]);
				}
				if (eachNode->children[1] != nullptr)
				{
					nodeStack.push(eachNode->children[1]);
				}
			}
		}
	}
	return resultList;
}

/**
* @brief
*  This is a query function to
*  check which objects is intersecting
*  with the ray (ray vs BVH to know)
* @param Ray const& ray
*  The ray to check with
* @param bool closest_only
*  the boolean to check if it want the closest only object
* @param std::vector<unsigned>& allIntersectedObjects
*  all intersected objects
* @param std::vector<Node const*>& debug_tested_nodes
*  list of tested nodes
* @return std::optional<unsigned>
*  if we want the closest, it will return the closest objects id
*/
template<typename T>
std::optional<unsigned> Bvh<T>::QueryDebug(Ray const& ray, bool closest_only, std::vector<unsigned>& allIntersectedObjects, std::vector<Node const*>& debug_tested_nodes) const
{
	// clear all the list before it starts
	allIntersectedObjects.clear();
	debug_tested_nodes.clear();

	std::optional<unsigned> closestID;
	float closestTime = std::numeric_limits<float>::infinity(); // start with largest value
	if (mRoot == nullptr)
	{
		return std::nullopt; // early return as there is a empty tree
	}
	std::stack<Node const*> nodeStack; // stack for depth first traversal, not using recursive
	nodeStack.push(mRoot);
	while (nodeStack.empty() == false) // main loop to visit every node
	{
		Node const* eachNode = nodeStack.top(); // get last node pushed
		nodeStack.pop();
		// record that this node is tested
		debug_tested_nodes.push_back(eachNode);
		float timeIntersectWithCurrentNodeBV = ray.intersect(eachNode->bv); // check the current intersect time
		if (timeIntersectWithCurrentNodeBV < 0.0f) // negative mean no intersect, skip the whole child (subtree)
		{
			continue;
		}
		// if we only want closest and the current node is bigger than what we had, is safe to skip because is impossible to beat
		if (closest_only && timeIntersectWithCurrentNodeBV >= closestTime)
		{
			continue;
		}
		if (eachNode->IsLeaf())
		{
			T object = eachNode->firstObject;
			while (object != nullptr)
			{
				float timeIntersectWithObject = ray.intersect(object->bv);
				if (timeIntersectWithObject < 0.0f)
				{
					object = object->bvhInfo.next;
					continue; // no intersect
				}
				else
				{
					allIntersectedObjects.push_back(object->id);
					// check if we have the closest one
					if (timeIntersectWithObject < closestTime)
					{
						closestTime = timeIntersectWithObject;
						closestID = object->id;
					}
				}
				object = object->bvhInfo.next;
			}
		}
		else // still in internal node
		{
			float timeIntersectLeftChild = (eachNode->children[0] != nullptr) ? ray.intersect(eachNode->children[0]->bv) : -1.0f;
			float timeIntersectRightChild = (eachNode->children[1] != nullptr) ? ray.intersect(eachNode->children[1]->bv) : -1.0f;
			// check if we need continue to check the subtree both left and right
			bool ifWeNeedCheckLeft = (timeIntersectLeftChild >= 0.0f) && (!closest_only || timeIntersectLeftChild < closestTime);
			bool ifWeNeedCheckRight = (timeIntersectRightChild >= 0.0f) && (!closest_only || timeIntersectRightChild < closestTime);
			if (ifWeNeedCheckLeft && ifWeNeedCheckRight)
			{
				if (timeIntersectLeftChild < timeIntersectRightChild) // is easier to find from left first, since the left is closer than right
				{
					// hence i last push left first, so later we will check left subtree first
					nodeStack.push(eachNode->children[1]);
					nodeStack.push(eachNode->children[0]);
				}
				else
				{
					nodeStack.push(eachNode->children[0]);
					nodeStack.push(eachNode->children[1]);
				}
			}
			else if (ifWeNeedCheckLeft == true && ifWeNeedCheckRight == false) // only check left
			{
				nodeStack.push(eachNode->children[0]);
			}
			else if (ifWeNeedCheckRight == true && ifWeNeedCheckLeft == false) // only check right
			{
				nodeStack.push(eachNode->children[1]);
			}
		}
	}
	return closestID;
}
/**
* @brief
*  This is a debug function will
*  traverses the tree node and use the
*  function given as input
* @param Fn func
*  function to call
* @param void
*
*/
template<typename T>
template<typename Fn>
void Bvh<T>::TraverseLevelOrder(Fn func) const
{
	if (mRoot != nullptr)
	{
		mRoot->TraverseLevelOrder(func); // just call the defined one in node to do the dirty job
	}
}

/**
* @brief
*  This is a debug function will
*  traverses the tree object and use the
*  function given as input
* @param Fn func
*  function to call
* @param void
*/
template<typename T>
template<typename Fn>
void Bvh<T>::TraverseLevelOrderObjects(Fn func) const
{
	if (mRoot != nullptr)
	{
		mRoot->TraverseLevelOrderObjects(func); // just call the defined one in node to do the dirty job
	}
}

/**
* @brief
*  This is a helper function that will
*  compute the whole aabb of a given objects
* @param std::vector<T> const& objects
*  the objects list
* @return Aabb
*  the calculated aabb
*/
template<typename T>
Aabb Bvh<T>::compute_aabb(std::vector<T> const& objects) const
{
	if (objects.empty())
	{
		return Aabb();
	}
	Aabb result = objects[0]->bv; // a bv to start with
	for (size_t i = 1; i < objects.size(); i++) // loop and find the min and max of all objects
	{
		result = Aabb(glm::min(result.min, objects[i]->bv.min), glm::max(result.max, objects[i]->bv.max));
	}
	return result;
}

/**
* @brief
*  This is a helper function that will
*  calculate the spatial median of the
*  bounding volume based on the axis
* @param Aabb const& bv
*  the bounding volume
* @param int axis
*  the axis
* @return float
*  the spatial median
*/
template<typename T>
float Bvh<T>::calculate_spatial_median(Aabb const& bv, int axis) const
{
	return (bv.min[axis] + bv.max[axis]) / 2.0f;
}

/**
* @brief
*  This is a function that will
*  build the bvh tree using top down
*  approach
* @param IT begin
*  object iterator begin
* @param IT end
*  object iterator end
* @param BvhBuildConfig const& config
*  required configuration
* @return void
*/
template<typename T>
template<typename IT>
void Bvh<T>::BuildTopDown(IT begin, IT end, BvhBuildConfig const& config)
{
	Clear(); // clear all the current bvh before building a new one
	mRoot = nullptr;
	mObjectCount = 0;

	if (begin == end) // empty obejcts
	{
		return;
	}

	std::vector<T> objects = std::vector<T>(begin, end); // get all the objects
	mObjectCount = static_cast<unsigned int>(objects.size());
	mRoot = BuildTopDownRecursive(objects, 0, config);
}

/**
* @brief
*  This is a helper function that will
*  help to build the bvh tree using top down
*  approach recursively, it splits with spatial
*  median
* @param std::vector<T>& objects
*  objects list
* @param unsigned depth
*  required depth
* @param BvhBuildConfig const& config
*  required configuration
* @return void
*/
template<typename T>
typename Bvh<T>::Node* Bvh<T>::BuildTopDownRecursive(std::vector<T>& objects, unsigned depth, BvhBuildConfig const& config)
{
	Node* node = new Node();
	node->bv = compute_aabb(objects);
	if (objects.size() <= config.minObjects || depth >= config.maxDepth || node->bv.volume() < config.minVolume)
	{
		node->children[0] = nullptr;
		node->children[1] = nullptr;
		node->firstObject = nullptr;
		node->lastObject = nullptr;

		for (T obj : objects)
		{
			node->AddObject(obj); // only leaf node store objects
		}
		return node; // hit the stop condition, is a leaf node, we can return now
	}
	else
	{
		// internal node shouldnt hold any objects
		node->firstObject = nullptr;
		node->lastObject = nullptr;
		// find the longest axis
		int longestAxis = node->bv.get_longest_axis();
		// find the spatial median
		float splitPosition = calculate_spatial_median(node->bv, longestAxis);
		// partitioning based on the objects center
		std::vector<T> leftObjects;
		std::vector<T> rightObjects;
		for (T object : objects)
		{
			if (object->bv.get_center()[longestAxis] < splitPosition)
			{
				leftObjects.push_back(object);
			}
			else
			{
				rightObjects.push_back(object);
			}
		}
		// handle edge case if all objects on one side
		if (leftObjects.empty() || rightObjects.empty())
		{
			node->children[0] = nullptr;
			node->children[1] = nullptr;
			for (T obj : objects)
			{
				node->AddObject(obj); // only leaf node store objects
			}
			return node;
		}
		// recursive call for two children
		node->children[0] = BuildTopDownRecursive(leftObjects, depth + 1, config);
		node->children[1] = BuildTopDownRecursive(rightObjects, depth + 1, config);
	}
	return node;
}

/**
* @brief
*  This is a function that will
*  build the bvh tree using insertion
*  approach, with the help of goldsmith-salmon
*  algorithm and rebalancing
* @param IT begin
*  object iterator begin
* @param IT end
*  object iterator end
* @param BvhBuildConfig const& config
*  required configuration
* @return void
*/
template<typename T>
template<typename IT>
void Bvh<T>::Insert(IT begin, IT end, BvhBuildConfig const& config)
{
	IT it = begin;
	while (it != end)
	{
		Insert(*it, config);
		it++;
	}
}

/**
* @brief
*  This is a function that will
*  build the bvh tree using insertion
*  approach, with the help of goldsmith-salmon
*  algorithm and rebalancing
* @param T object
*  object to add
* @param BvhBuildConfig const& config
*  required configuration
* @return void
*/
template<typename T>
void Bvh<T>::Insert(T object, BvhBuildConfig const& config)
{
	enum class InsertChoice
	{
		PairUnderNew,
		Recurse,
		AddToExisting // for identical AABB
	};
	InsertChoice choice = InsertChoice::Recurse;
	if (mRoot == nullptr)
	{
		mRoot = new Node(); // create new leaf node to hold the object since there is no tree yet
		mRoot->bv = object->bv;
		mRoot->children[0] = nullptr;
		mRoot->children[1] = nullptr;
		mRoot->firstObject = nullptr;
		mRoot->lastObject = nullptr;
		mRoot->AddObject(object);
		mObjectCount = 1;
		return;
	}
	// keep track of nodes visited
	std::stack<Node*> nodePath;
	Node* eachNode = mRoot;
	unsigned depth = 0;
	while (eachNode->IsLeaf() == false) // run all the way until is leaf
	{
		if (depth >= config.maxDepth || eachNode->bv.volume() < config.minVolume)
		{
			choice = InsertChoice::PairUnderNew;
			break;
		}
		// precompute surface areas and merge them
		float oldSurfaceArea = eachNode->bv.surface_area();
		float surfaceAreaLeftChild = eachNode->children[0]->bv.surface_area();
		float surfaceAreaRightChild = eachNode->children[1]->bv.surface_area();
		float newSurfaceArea = eachNode->bv.Merge(object->bv).surface_area();
		float newSurfaceAreaLeftChild = eachNode->children[0]->bv.Merge(object->bv).surface_area();
		float newSurfaceAreaRightChild = eachNode->children[1]->bv.Merge(object->bv).surface_area();
		// three different choice for insertion
		float costMakeNewInternal = 2 * newSurfaceArea;
		float costAdd = 3 * newSurfaceArea - oldSurfaceArea;
		float costRecursive = std::min(newSurfaceAreaLeftChild + surfaceAreaRightChild, surfaceAreaLeftChild + newSurfaceAreaRightChild);

		// pick the cheapest option
		if (costMakeNewInternal <= costAdd && costMakeNewInternal <= costRecursive)
		{
			choice = InsertChoice::PairUnderNew;
			break;
		}
		else if (costAdd <= costRecursive)
		{
			choice = InsertChoice::AddToExisting;
		}
		else // better to drill down to check which children give lower sah increase
		{
			if (newSurfaceAreaLeftChild + surfaceAreaRightChild < surfaceAreaLeftChild + newSurfaceAreaRightChild)
			{
				nodePath.push(eachNode);
				eachNode = eachNode->children[0];
			}
			else
			{
				nodePath.push(eachNode);
				eachNode = eachNode->children[1];
			}
			depth++;
			continue;
		}

	}
	// handle we actually at leaf
	if (eachNode->IsLeaf())
	{
		// add to existing for identical aabb, no need to create new node
		float epsilon = 1e-6f;
		if (std::abs(eachNode->bv.min.x - object->bv.min.x) < epsilon &&
			std::abs(eachNode->bv.min.y - object->bv.min.y) < epsilon &&
			std::abs(eachNode->bv.min.z - object->bv.min.z) < epsilon &&
			std::abs(eachNode->bv.max.x - object->bv.max.x) < epsilon &&
			std::abs(eachNode->bv.max.y - object->bv.max.y) < epsilon &&
			std::abs(eachNode->bv.max.z - object->bv.max.z) < epsilon)
		{
			choice = InsertChoice::AddToExisting;
		}
		else
		{
			choice = InsertChoice::PairUnderNew;
		}
	}
	// the new leaf to add
	if (choice == InsertChoice::AddToExisting)
	{
		// add it in existing leaf node
		eachNode->AddObject(object);
	}
	else
	{
		// create the new leaf
		Node* newLeaf = new Node();
		newLeaf->children[0] = nullptr;
		newLeaf->children[1] = nullptr;
		newLeaf->firstObject = nullptr;
		newLeaf->lastObject = nullptr;
		newLeaf->bv = object->bv;
		newLeaf->AddObject(object);
		// prepare the new internal pairing the old leaf (current) (each node) and the new leaf
		Node* newInternal = nullptr;
		if (choice == InsertChoice::PairUnderNew)
		{
			newInternal = new Node();
			newInternal->children[0] = eachNode; // old leaf
			newInternal->children[1] = newLeaf; // new leaf
			newInternal->firstObject = nullptr;
			newInternal->lastObject = nullptr;
			newInternal->bv = eachNode->bv.Merge(object->bv);
			// merge it into the parent or replace the root
			if (nodePath.empty())
			{
				mRoot = newInternal;
			}
			else
			{
				Node* parent = nodePath.top(); // original parent node in stack
				if (parent->children[0] == eachNode)
				{
					parent->children[0] = newInternal;
				}
				else
				{
					parent->children[1] = newInternal;
				}
			}
			nodePath.push(newInternal);
		}
	}
	++mObjectCount;

	// periodic rebalancing: every 200 insertions, and check
	// if the current tree depth is much larger than the ideal (which for a perfectly balanced binary tree is ~log₂(N)
	if (mObjectCount % 200 == 0 && Depth() > 2 * log2(mObjectCount) + 5)
	{
		// tree is unbalanced, rebuild it using balanced top-down construction
		// this is to remains an optimal tree
		std::vector<T> allObjects;
		TraverseLevelOrderObjects([&](T obj)
			{
				allObjects.push_back(obj);
			});
		Clear();
		BuildTopDown(allObjects.begin(), allObjects.end(), config);
		// Clear the node path since old nodes are now invalid
		while (!nodePath.empty())
		{
			nodePath.pop();
		}
		return; // skip the refitting since tree was rebuilt
	}

	while (nodePath.empty() == false) // refit all the ancestor bv
	{
		Node* node = nodePath.top();
		nodePath.pop();

		// make sure both children exist before merging
		if (node->children[0] != nullptr && node->children[1] != nullptr)
		{
			node->bv = node->children[0]->bv.Merge(node->children[1]->bv);
		}
	}
}

/**
* @brief
*  This function removes an object from the BVH
*  It removes the object from the linked list, and if the leaf
*  becomes empty, removes the leaf and promotes its sibling
* @param T object
*  the object to remove
* @return void
*/
template<typename T>
void Bvh<T>::Remove(T object)
{
	// validate if the object is in the tree
	if (object == nullptr || object->bvhInfo.node == nullptr)
	{
		return; // object not in tree
	}

	Node* leafNode = object->bvhInfo.node;

	// remove object from the linked list in the leaf node
	if (object->bvhInfo.prev != nullptr)
	{
		object->bvhInfo.prev->bvhInfo.next = object->bvhInfo.next;
	}
	else
	{
		// the object is first in list
		leafNode->firstObject = object->bvhInfo.next;
	}

	if (object->bvhInfo.next != nullptr)
	{
		object->bvhInfo.next->bvhInfo.prev = object->bvhInfo.prev;
	}
	else
	{
		// the object is last in list
		leafNode->lastObject = object->bvhInfo.prev;
	}

	// clear object's BVH info
	object->bvhInfo.node = nullptr;
	object->bvhInfo.prev = nullptr;
	object->bvhInfo.next = nullptr;
	--mObjectCount;

	//----------LEAF Parts----------------------------------
	// case 1: leaf still has objects - refit AABB and ancestors
	if (leafNode->firstObject != nullptr)
	{
		// recompute leaf AABB from remaining objects
		std::vector<T> remainingObjects;
		T obj = leafNode->firstObject;
		while (obj != nullptr)
		{
			remainingObjects.push_back(obj);
			obj = obj->bvhInfo.next;
		}
		leafNode->bv = compute_aabb(remainingObjects);

		// refit ancestors - need to find path from root to this leaf
		if (mRoot != leafNode)
		{
			std::stack<Node*> pathToLeaf;
			std::function<bool(Node*)> findAndRefitPath = [&](Node* current) -> bool
			{
				if (current == leafNode)
				{
					return true;
				}
				if (current->IsLeaf())
				{
					return false;
				}
				pathToLeaf.push(current);

				// try left child
				if (current->children[0] && findAndRefitPath(current->children[0]))
				{
					// Found it in left subtree, refit on way back up
					current->bv = current->children[0]->bv.Merge(current->children[1]->bv);
					return true;
				}

				// Try right child
				if (current->children[1] && findAndRefitPath(current->children[1]))
				{
					// Found it in right subtree, refit on way back up
					current->bv = current->children[0]->bv.Merge(current->children[1]->bv);
					return true;
				}

				pathToLeaf.pop();
				return false;
			};
			findAndRefitPath(mRoot);
		}
		return;
	}

	// Case 2: leaf is root and now empty - need to remove it from tree
	if (leafNode == mRoot)
	{
		// removed last object in tree
		delete mRoot;
		mRoot = nullptr;
		return;
	}

	// case 3: find parent and replace it with sibling
	// this removes the empty leaf and promotes the sibling up
	std::function<bool(Node*&)> findAndRemoveEmptyLeaf = [&](Node*& nodeRef) -> bool
	{
		Node* current = nodeRef;

		if (current == nullptr || current->IsLeaf())
		{
			return false;
		}

		// Check if one of our children is the empty leaf
		if (current->children[0] == leafNode)
		{
			// Left child is empty leaf, promote right child
			Node* sibling = current->children[1];
			delete leafNode;
			delete current;
			nodeRef = sibling;
			return true;
		}
		else if (current->children[1] == leafNode)
		{
			// Right child is empty leaf, promote left child
			Node* sibling = current->children[0];
			delete leafNode;
			delete current;
			nodeRef = sibling;
			return true;
		}

		// Recurse into left subtree
		if (findAndRemoveEmptyLeaf(current->children[0]))
		{
			// Refit this node after removal
			if (current->children[0] && current->children[1])
			{
				current->bv = current->children[0]->bv.Merge(current->children[1]->bv);
			}
			return true;
		}

		// Recurse into right subtree
		if (findAndRemoveEmptyLeaf(current->children[1]))
		{
			// Refit this node after removal
			if (current->children[0] && current->children[1])
			{
				current->bv = current->children[0]->bv.Merge(current->children[1]->bv);
			}
			return true;
		}

		return false;
	};

	findAndRemoveEmptyLeaf(mRoot);
}

#endif // BVH_INL
