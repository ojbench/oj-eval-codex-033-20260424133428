#ifndef SJTU_LIST_HPP
#define SJTU_LIST_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace sjtu {
/**
 * A minimal std::list-like container with bidirectional iterators.
 */
template <typename T> class list {
protected:
  struct base_node {
    base_node *prev{nullptr};
    base_node *next{nullptr};
  };
  struct node : base_node {
    T value;
    template <typename U>
    explicit node(U &&v) : base_node{nullptr, nullptr}, value(std::forward<U>(v)) {}
  };

  base_node *m_head; // circular sentinel (no T)
  size_t m_size;

  node *create_node(const T &v) { return new node(v); }

  // insert node cur before node pos
  base_node *insert(base_node *pos, base_node *cur) {
    cur->prev = pos->prev;
    cur->next = pos;
    pos->prev->next = cur;
    pos->prev = cur;
    ++m_size;
    return cur;
  }

  // remove node pos from list (no delete here)
  base_node *erase(base_node *pos) {
    base_node *nxt = pos->next;
    pos->prev->next = pos->next;
    pos->next->prev = pos->prev;
    pos->prev = pos->next = nullptr;
    --m_size;
    return nxt;
  }

  void init_empty() {
    m_head = new base_node();
    m_head->prev = m_head->next = m_head;
    m_size = 0;
  }

  void destroy_all() {
    while (!empty()) {
      base_node *cur = m_head->next;
      erase(cur);
      delete static_cast<node *>(cur);
    }
    delete m_head;
    m_head = nullptr;
  }

public:
  class const_iterator;
  class iterator {
    friend class list;
    friend class const_iterator;
    base_node *ptr;
    const list *owner;

    iterator(base_node *p, const list *o) : ptr(p), owner(o) {}

  public:
    iterator() : ptr(nullptr), owner(nullptr) {}

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    iterator &operator++() {
      if (!owner || !ptr) throw std::out_of_range("invalid iterator");
      ptr = ptr->next;
      return *this;
    }
    iterator operator--(int) {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }
    iterator &operator--() {
      if (!owner || !ptr) throw std::out_of_range("invalid iterator");
      ptr = ptr->prev;
      return *this;
    }

    T &operator*() const {
      if (!owner || !ptr || ptr == owner->m_head)
        throw std::out_of_range("dereferencing end or invalid iterator");
      return static_cast<node *>(ptr)->value;
    }
    T *operator->() const noexcept { return &(**this); }

    bool operator==(const iterator &rhs) const { return ptr == rhs.ptr; }
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
    bool operator==(const const_iterator &rhs) const;
    bool operator!=(const const_iterator &rhs) const;
  };

  class const_iterator {
    friend class list;
    const base_node *ptr;
    const list *owner;

    const_iterator(const base_node *p, const list *o) : ptr(p), owner(o) {}

  public:
    const_iterator() : ptr(nullptr), owner(nullptr) {}
    const_iterator(const iterator &it) : ptr(it.ptr), owner(it.owner) {}

    const_iterator operator++(int) {
      const_iterator tmp = *this; ++(*this); return tmp;
    }
    const_iterator &operator++() {
      if (!owner || !ptr) throw std::out_of_range("invalid iterator");
      ptr = ptr->next;
      return *this;
    }
    const_iterator operator--(int) {
      const_iterator tmp = *this; --(*this); return tmp;
    }
    const_iterator &operator--() {
      if (!owner || !ptr) throw std::out_of_range("invalid iterator");
      ptr = ptr->prev;
      return *this;
    }

    const T &operator*() const {
      if (!owner || !ptr || ptr == owner->m_head)
        throw std::out_of_range("dereferencing end or invalid iterator");
      return static_cast<const node *>(ptr)->value;
    }
    const T *operator->() const noexcept { return &(**this); }

    bool operator==(const const_iterator &rhs) const { return ptr == rhs.ptr; }
    bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
    bool operator==(const iterator &rhs) const { return ptr == rhs.ptr; }
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
  };

  // cross comparisons implemented inline in class bodies

  // Constructors, destructor, assignment
  list() { init_empty(); }

  list(const list &other) { init_empty(); for (const T &v : other) push_back(v); }

  ~list() { destroy_all(); }

  list &operator=(const list &other) {
    if (this == &other) return *this;
    clear();
    for (const T &v : other) push_back(v);
    return *this;
  }

  // element access
  const T &front() const {
    if (empty()) throw std::out_of_range("list is empty");
    return static_cast<node *>(m_head->next)->value;
  }
  const T &back() const {
    if (empty()) throw std::out_of_range("list is empty");
    return static_cast<node *>(m_head->prev)->value;
  }
  T &front() { return const_cast<T &>(static_cast<const list &>(*this).front()); }
  T &back()  { return const_cast<T &>(static_cast<const list &>(*this).back()); }

  // iterators
  iterator begin() { return iterator(m_head->next, this); }
  const_iterator cbegin() const { return const_iterator(m_head->next, this); }
  iterator end() { return iterator(m_head, this); }
  const_iterator cend() const { return const_iterator(m_head, this); }

  // capacity
  bool empty() const { return m_size == 0; }
  size_t size() const { return m_size; }

  // modifiers
  void clear() {
    while (!empty()) { base_node *cur = m_head->next; erase(cur); delete static_cast<node *>(cur); }
  }

  iterator insert(iterator pos, const T &value) {
    if (pos.owner != this) throw std::out_of_range("iterator from different list");
    node *n = create_node(value);
    base_node *res = insert(pos.ptr, n);
    return iterator(res, this);
  }

  iterator erase(iterator pos) {
    if (pos.owner != this || pos.ptr == m_head) throw std::out_of_range("invalid erase");
    base_node *nxt = erase(pos.ptr);
    delete static_cast<node *>(pos.ptr);
    return iterator(nxt, this);
  }

  void push_back(const T &value) { insert(end(), value); }
  void push_front(const T &value) { insert(begin(), value); }

  void pop_back() {
    if (empty()) throw std::out_of_range("pop_back on empty list");
    iterator it(m_head->prev, this); erase(it);
  }
  void pop_front() {
    if (empty()) throw std::out_of_range("pop_front on empty list");
    iterator it(m_head->next, this); erase(it);
  }

  // range-for support
  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }
};

} // namespace sjtu

#endif // SJTU_LIST_HPP
