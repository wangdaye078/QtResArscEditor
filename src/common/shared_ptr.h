/**
 * @file  shared_ptr.hpp
 * @brief shared_ptr is a minimal implementation of smart pointer, a subset of the C++11 std::shared_ptr or boost::shared_ptr.
 *
 * Copyright (c) 2013-2019 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
 //********************************************************************
 //	filename: 	F:\mygit\QtResArscEditor2\src\common\shared_ptr.h
 //	desc:		原来的引用计数只是一个long，而我为了从一个智能指针找出指向同一目标的所有智能指针
 //				所以做了修改，把引用计数改成了std::set，通过set.size取得计数，set本身则包含了所有
 //             指向同一目标的智能指针的指针
 //
 //	created:	基于https://github.com/SRombauts/shared_ptr/blob/master/include/shared_ptr.hpp
 //********************************************************************
#ifndef shared_ptr_h__
#define shared_ptr_h__
#include <set>
#include <cstddef>      // NULL
#include <algorithm>    // std::swap
// can be replaced by other error mechanism
#include <cassert>
#define SHARED_ASSERT(x)    assert(x)

namespace SRombauts
{

	template<class>
	class shared_ptr;
	/**
	 * @brief implementation of reference counter for the following minimal smart pointer.
	 *
	 * shared_ptr_count is a container for the allocated pn reference counter.
	 */

	template<class T>
	class shared_ptr_count
	{
	public:
		shared_ptr_count() :
			ppn(NULL)
		{
		}
		shared_ptr_count(const shared_ptr_count& count) :
			ppn(count.ppn)
		{
		}
		/// @brief Swap method for the copy-and-swap idiom (copy constructor and swap method)
		template<class U>
		void swap(shared_ptr<U>* l, shared_ptr<U>* r) noexcept // never throws
		{
			if (l->pn.ppn != NULL)
				l->pn.ppn->erase(l);
			if (r->pn.ppn != NULL)
				r->pn.ppn->erase(r);
			std::swap(l->pn.ppn, r->pn.ppn);
			if (l->pn.ppn != NULL)
				l->pn.ppn->insert(l);
			if (r->pn.ppn != NULL)
				r->pn.ppn->insert(r);
		}
		/// @brief getter of the underlying reference counter
		size_t use_count(void) const noexcept // never throws
		{
			size_t count = 0;
			if (NULL != ppn)
			{
				count = ppn->size();
			}
			return count;
		}
		/// @brief acquire/share the ownership of the pointer, initializing the reference counter
		template<class U>
		void acquire(shared_ptr<U>* p) // may throw std::bad_alloc
		{
			if (NULL != p->get())
			{
				if (NULL == ppn)
				{
					try
					{
						ppn = new std::set<shared_ptr<T>*>; // may throw std::bad_alloc
						ppn->insert(p);
					}
					catch (std::bad_alloc&)
					{
						delete p->get();
						throw; // rethrow the std::bad_alloc
					}
				}
				else
				{
					ppn->insert(p);
				}
			}
		}
		/// @brief release the ownership of the px pointer, destroying the object when appropriate
		template<class U>
		void release(shared_ptr<U>* p) noexcept // never throws
		{
			if (NULL != ppn)
			{
				ppn->erase(p);
				if (0 == ppn->size())
				{
					delete p->get();
					delete ppn;
				}
				ppn = NULL;
			}
		}

	public:
		std::set<shared_ptr<T>*>* ppn; //!< Reference counter
	};

	template<class T>
	class shared_ptr_base
	{
		friend class shared_ptr_count<T>;	// allow shared_ptr_count to access the protected members
	protected:
		shared_ptr_base(void) :
			pn()
		{
		}

		shared_ptr_base(const shared_ptr_base& other) :
			pn(other.pn)
		{
		}
		shared_ptr_count<T>    pn; //!< Reference counter
	};


	/**
	 * @brief minimal implementation of smart pointer, a subset of the C++11 std::shared_ptr or boost::shared_ptr.
	 *
	 * shared_ptr is a smart pointer retaining ownership of an object through a provided pointer,
	 * and sharing this ownership with a reference counter.
	 * It destroys the object when the last shared pointer pointing to it is destroyed or reset.
	 */
	template<class T>
	class shared_ptr : public shared_ptr_base<T>
	{
	public:
		/// The type of the managed object, aliased as member type
		typedef T element_type;

		/// @brief Default constructor
		shared_ptr(void) noexcept : // never throws
			shared_ptr_base<T>(),
			px(NULL)
		{
		}
		/// @brief Constructor with the provided pointer to manage
		explicit shared_ptr(T* p) : // may throw std::bad_alloc
			//px(p), would be unsafe as acquire() may throw, which would call release() in destructor
			shared_ptr_base<T>()
		{
			acquire(p);   // may throw std::bad_alloc
		}
		/// @brief Constructor to share ownership. Warning : to be used for pointer_cast only ! (does not manage two separate <T> and <U> pointers)
		template <class U>
		shared_ptr(const shared_ptr<U>& ptr, T* p) :
			//px(p), would be unsafe as acquire() may throw, which would call release() in destructor
			shared_ptr_base<T>(ptr)
		{
			acquire(p);   // may throw std::bad_alloc
		}
		/// @brief Copy constructor to convert from another pointer type
		template <class U>
		shared_ptr(const shared_ptr<U>& ptr) noexcept : // never throws (see comment below)
			//px(ptr.px),
			shared_ptr_base<T>(ptr)
		{
			SHARED_ASSERT((NULL == ptr.get()) || (0 != ptr.use_count())); // must be coherent : no allocation allowed in this path
			acquire(static_cast<typename shared_ptr<T>::element_type*>(ptr.get()));   // will never throw std::bad_alloc
		}
		/// @brief Copy constructor (used by the copy-and-swap idiom)
		shared_ptr(const shared_ptr& ptr) noexcept : // never throws (see comment below)
			//px(ptr.px),
			shared_ptr_base<T>(ptr)
		{
			SHARED_ASSERT((NULL == ptr.px) || (0 != ptr.pn.use_count())); // must be cohérent : no allocation allowed in this path
			acquire(ptr.px);   // will never throw std::bad_alloc
		}
		/// @brief Assignment operator using the copy-and-swap idiom (copy constructor and swap method)
		shared_ptr& operator=(shared_ptr ptr) noexcept // never throws
		{
			swap(ptr);
			return *this;
		}
		/// @brief the destructor releases its ownership
		~shared_ptr(void) noexcept // never throws
		{
			release();
		}
		/// @brief this reset releases its ownership
		void reset(void) noexcept // never throws
		{
			release();
		}
		/// @brief this reset release its ownership and re-acquire another one
		void reset(T* p) // may throw std::bad_alloc
		{
			SHARED_ASSERT((NULL == p) || (px != p)); // auto-reset not allowed
			release();
			acquire(p); // may throw std::bad_alloc
		}

		/// @brief Swap method for the copy-and-swap idiom (copy constructor and swap method)
		void swap(shared_ptr& lhs) noexcept // never throws
		{
			if (lhs.pn.ppn == shared_ptr_base<T>::pn.ppn)
				return; // nothing to do, same pointer

			std::swap(px, lhs.px);
			shared_ptr_base<T>::pn.swap(this, &lhs);
		}

		// reference counter operations :
		operator bool() const noexcept // never throws
		{
			return (0 < shared_ptr_base<T>::pn.use_count());
		}
		bool unique(void)  const noexcept // never throws
		{
			return (1 == shared_ptr_base<T>::pn.use_count());
		}
		size_t use_count(void)  const noexcept // never throws
		{
			return shared_ptr_base<T>::pn.use_count();
		}
		std::set<shared_ptr<T>*> use_ref(void)  const noexcept // never throws
		{
			if (shared_ptr_base<T>::pn.ppn == NULL)
				return std::set<shared_ptr<T>*>();
			else
				return *shared_ptr_base<T>::pn.ppn;
		}
		// underlying pointer operations :
		T& operator*()  const noexcept // never throws
		{
			SHARED_ASSERT(NULL != px);
			return *px;
		}
		T* operator->() const noexcept // never throws
		{
			SHARED_ASSERT(NULL != px);
			return px;
		}
		T* get(void)  const noexcept // never throws
		{
			// no assert, can return NULL
			return px;
		}

	private:
		/// @brief acquire/share the ownership of the px pointer, initializing the reference counter
		void acquire(T* p) // may throw std::bad_alloc
		{
			px = p; // here it is safe to acquire the ownership of the provided raw pointer, where exception cannot be thrown any more
			shared_ptr_base<T>::pn.acquire(this); // may throw std::bad_alloc
		}

		/// @brief release the ownership of the px pointer, destroying the object when appropriate
		void release(void) noexcept // never throws
		{
			shared_ptr_base<T>::pn.release(this);
			px = NULL;
		}

	private:
		T* px; //!< Native pointer
	};


	// comparaison operators
	template<class T, class U> bool operator==(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept // never throws
	{
		return (l.get() == r.get());
	}
	template<class T, class U> bool operator!=(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept // never throws
	{
		return (l.get() != r.get());
	}
	template<class T, class U> bool operator<=(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept // never throws
	{
		return (l.get() <= r.get());
	}
	template<class T, class U> bool operator<(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept // never throws
	{
		return (l.get() < r.get());
	}
	template<class T, class U> bool operator>=(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept // never throws
	{
		return (l.get() >= r.get());
	}
	template<class T, class U> bool operator>(const shared_ptr<T>& l, const shared_ptr<U>& r) noexcept // never throws
	{
		return (l.get() > r.get());
	}



	// static cast of shared_ptr
	template<class T, class U>
	shared_ptr<T> static_pointer_cast(const shared_ptr<U>& ptr) // never throws
	{
		return shared_ptr<T>(ptr, static_cast<typename shared_ptr<T>::element_type*>(ptr.get()));
	}

	// dynamic cast of shared_ptr
	template<class T, class U>
	shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& ptr) // never throws
	{
		T* p = dynamic_cast<typename shared_ptr<T>::element_type*>(ptr.get());
		if (NULL != p)
		{
			return shared_ptr<T>(ptr, p);
		}
		else
		{
			return shared_ptr<T>();
		}
	}

}

#endif // shared_ptr_h__
