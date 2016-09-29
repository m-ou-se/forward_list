#pragma once

#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>

template<typename T>
class forward_list {

	struct node;

	struct node_base {
		std::unique_ptr<node> next;
	};

	struct node : node_base {
		T value;
		template<typename ... Args>
		explicit node(Args && ... args) : value(std::forward<Args>(args)...) {}
	};

	node_base before_begin_;

public:
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = value_type const &;
	using pointer = value_type *;
	using const_pointer = value_type const *;

	class iterator {
		node_base * node_ = nullptr;
		using node_type = node;
	public:
		using value_type = T;

	#define iterator_impl(iterator) \
		private: \
		explicit iterator(decltype(node_) node) : node_(node) {} \
		public: \
		using pointer = value_type *; \
		using reference = value_type &; \
		using difference_type = std::ptrdiff_t; \
		using iterator_category = std::forward_iterator_tag; \
		iterator() {} \
		reference operator * () const { return static_cast<node_type *>(node_)->value; } \
		pointer operator -> () const { return &**this; } \
		iterator & operator ++ () { node_ = node_->next.get(); return *this; } \
		iterator operator ++ (int) { iterator i = *this; ++*this; return i; } \
		friend bool operator == (iterator a, iterator b) { return a.node_ == b.node_; } \
		friend bool operator != (iterator a, iterator b) { return a.node_ != b.node_; } \
		friend class forward_list;

		iterator_impl(iterator)
	};

	class const_iterator {
		node_base const * node_ = nullptr;
		using node_type = node const;
	public:
		using value_type = T const;
		const_iterator(iterator i) : node_(i.node_) {}

		iterator_impl(const_iterator)

		#undef iterator_impl
	};

	forward_list() {}

	forward_list(size_t count, T const & value) {
		auto i = before_begin();
		while (count--) i = emplace_after(i, value);
	}

	forward_list(size_t count) {
		auto i = before_begin();
		while (count--) i = emplace_after(i);
	}

	// TODO: enable_if check for InputIterator.
	template<typename It>
	forward_list(It first, It last) {
		auto i = before_begin();
		for (It it = first; it != last; ++it) i = emplace_after(i, *it);
	}

	forward_list(std::initializer_list<T> init)
		: forward_list(init.begin(), init.end()) {}

	~forward_list() {
		clear();
	}

	iterator        before_begin()       { return       iterator(&before_begin_); }
	const_iterator  before_begin() const { return const_iterator(&before_begin_); }
	const_iterator cbefore_begin() const { return const_iterator(&before_begin_); }

	iterator        begin()       { return std::next(before_begin()); }
	const_iterator  begin() const { return std::next(before_begin()); }
	const_iterator cbegin() const { return std::next(before_begin()); }

	iterator        end()       { return {}; }
	const_iterator  end() const { return {}; }
	const_iterator cend() const { return {}; }

	iterator access(const_iterator i) {
		return iterator(const_cast<node_base *>(i.node_));
	}

	bool empty() const {
		return begin() == end();
	}

	T       & front()       { return *begin(); }
	T const & front() const { return *begin(); }

	void clear() {
		while (!empty()) pop_front();
	}

	void push_front(T const & value) {
		emplace_front(value);
	}

	void push_front(T && value) {
		emplace_front(std::move(value));
	}

	template<typename ... Args>
	T & emplace_front(Args && ... args) {
		return emplace_after(before_begin(), std::forward<Args>(args)...);
	}

	void pop_front() {
		erase_after(before_begin());
	}

	iterator insert_after(const_iterator pos, T const & value) {
		return emplace_after(pos, value);
	}

	iterator insert_after(const_iterator pos, T && value) {
		return emplace_after(pos, std::move(value));
	}

	iterator insert_after(const_iterator pos, size_t count, T const & value) {
		return splice_after(pos, forward_list(count, value));
	}

	// TODO: enable_if check for InputIterator.
	template<typename It>
	iterator insert_after(const_iterator pos, It first, It last) {
		return splice_after(pos, forward_list(first, last));
	}

	iterator insert_after(const_iterator pos, std::initializer_list<T> ilist) {
		return insert_after(pos, ilist.begin(), ilist.end());
	}

	template<typename ... Args>
	iterator emplace_after(const_iterator pos, Args && ... args) {
		auto i = access(pos);
		auto n = std::make_unique<node>(std::forward<Args>(args)...);
		n->next = std::move(i.node_->next);
		i.node_->next = std::move(n);
		return iterator(i.node_->next.get());
	}

	iterator erase_after(const_iterator pos) {
		auto i = access(pos);
		i.node_->next = std::move(i.node_->next->next);
		return iterator(i.node_->next.get());
	}

	iterator erase_after(const_iterator first, const_iterator last) {
		iterator i;
		while (std::next(first) != last) i = erase_after(first);
		return i;
	}

	void resize(size_t size) {
		auto i = before_begin();
		while (size && std::next(i) != end()) { --size; ++i; }
		erase_after(i, end());
		while (size--) i = emplace_after(i);
	}

	void resize(size_t size, T const * value) {
		auto i = before_begin();
		while (size && std::next(i) != end()) { --size; ++i; }
		erase_after(i, end());
		while (size--) i = emplace_after(i, value);
	}

	iterator splice_after(const_iterator pos, forward_list & other) {
		return splice_after(pos, other, other.before_begin(), other.end());
	}

	iterator splice_after(const_iterator pos, forward_list && other) {
		return splice_after(pos, other);
	}

	iterator splice_after(const_iterator pos, forward_list & other, const_iterator it) {
		auto from = other.access(it);
		auto to = access(pos);
		auto n = std::move(from.node_->next);
		from.node_->next = std::move(n->next);
		n->next = std::move(to.node_->next);
		to.node_->next = std::move(n);
		return std::next(to);
	}

	iterator splice_after(const_iterator pos, forward_list && other, const_iterator it) {
		return splice_after(pos, other, it);
	}

	iterator splice_after(const_iterator pos, forward_list & other, const_iterator first, const_iterator last) {
		if (std::next(pos) == end()) {
			// When splicing at the end, do it in O(1).
			access(pos).node_->next = std::move(other.access(first).node_->next);
		} else {
			while (std::next(first) != last) pos = splice_after(pos, other, first);
		}
		return access(pos);
	}

	iterator splice_after(const_iterator pos, forward_list && other, const_iterator first, const_iterator last) {
		return splice_after(pos, other, first, last);
	}

	void swap(forward_list & other) noexcept {
		std::swap(before_begin_.next, other.before_begin_.next);
	}
};
