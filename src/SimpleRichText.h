//********************************************************************
//	filename: 	F:\mygit\QtResArscEditor\src\SimpleRichText.h
//	desc:		
//
//	created:	wangdaye 17:3:2025   9:11
//********************************************************************
#ifndef SimpleRichText_h__
#define SimpleRichText_h__
#include <QMap>
#include "StringPoolExtend.h"

//需要修改QMap.h文件，找到friend class QMultiMap<Key, T>，在后面加入friend class QMultiMap_modify<Key, T>;大概有3处
template <class Key, class T>
class QMultiMap_modify : public QMultiMap<Key, T>
{
public:
	using typename QMap<Key, T>::Node;

	template <class Key, class T>
	typename QMap<Key, T>::iterator insert_modify(typename QMap<Key, T>::const_iterator pos,
		const Key& akey, const T& avalue)
	{
		if (this->d->ref.isShared())
			return this->insert(akey, avalue);

		Q_ASSERT_X(this->isValidIterator(pos), "QMap::insert", "The specified const_iterator argument 'pos' is invalid");

		if (pos == this->constEnd()) {
			// Hint is that the Node is larger than (or equal to) the largest value.
			Node* n = static_cast<Node*>(pos.i->left);
			if (n) {
				while (n->right)
					n = static_cast<Node*>(n->right);

				if (!qMapLessThanKey(n->key, akey))
					return this->insert(akey, avalue); // ignore hint
				Node* z = this->d->createNode(akey, avalue, n, false); // insert right most
				return typename QMap<Key, T>::iterator(z);
			}
			return this->insert(akey, avalue);
		}
		else {
			// Hint indicates that the node should be less (or equal to) the hint given
			// but larger than the previous value.
			Node* next = const_cast<Node*>(pos.i);
			if (qMapLessThanKey(next->key, akey))
				return this->insert(akey, avalue); // ignore hint

			if (pos == this->constBegin()) {
				// There is no previous value (insert left most)
				Node* z = this->d->createNode(akey, avalue, this->begin().i, true);
				return typename QMap<Key, T>::iterator(z);
			}
			else {
				Node* prev = const_cast<Node*>(pos.i->previousNode());
				//此处有修改，原来必须(pos - 1).key() < key <= pos.key()，现在改成(pos - 1).key() <= key <= pos.key()
				//也就是可以插入到相同key的任意一处，可以是最前面，也可以是最后面，甚至是中间的指定位置。
				//无法理解原来的逻辑，即使不加那个pos，应该也是加到那个位置吧，那把pos做参数还有什么意义呢？难道原来插的位置是随机的？
				//if (!qMapLessThanKey(prev->key, akey))
				if (qMapLessThanKey(akey, prev->key))
					return this->insert(akey, avalue); // ignore hint

				// Hint is ok - do insert
				if (prev->right == nullptr) {
					Node* z = this->d->createNode(akey, avalue, prev, false);
					return typename QMap<Key, T>::iterator(z);
				}
				if (next->left == nullptr) {
					Node* z = this->d->createNode(akey, avalue, next, true);
					return typename QMap<Key, T>::iterator(z);
				}
				Q_ASSERT(false); // We should have prev->right == nullptr or next->left == nullptr.
				return this->insert(akey, avalue);
			}
		}
	}
};

#endif // SimpleRichText_h__