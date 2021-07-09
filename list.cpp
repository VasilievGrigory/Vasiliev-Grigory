#include <iostream>
#include <list>
#include <vector>
#include <cmath>
#include <deque>
#include <stack>


template <size_t chunkSize>
class FixedAllocator {
public:
    std::stack<int8_t*> dealloc_memory;
    std::vector<int8_t*> pools_of_memory;
    size_t current_allocate_point = 0;
    static const int size = 64;
    int* how_many_ptrs;

    template<size_t SZ>
    friend bool operator==(const FixedAllocator<SZ>&, const FixedAllocator<SZ>&);

    FixedAllocator() {
        how_many_ptrs = new int(1);
    }
    FixedAllocator(const FixedAllocator&) {
        how_many_ptrs = new int(1);
    }
    FixedAllocator& operator=(const FixedAllocator&) {
        this->~FixedAllocator();
        how_many_ptrs = new int(1);
        return *this;
    }

    void new_memory_pool() {
        current_allocate_point = 0;
        pools_of_memory.push_back(new int8_t[size * chunkSize]);
    }

     void* allocate() {
        if (dealloc_memory.size() != 0) {
            auto memory_for_work = static_cast<void*>(dealloc_memory.top());
            dealloc_memory.pop();
            return memory_for_work;
        }

        if (current_allocate_point >= size * chunkSize || pools_of_memory.size() == 0) {
            new_memory_pool();
        }
        auto memory_for_work = static_cast<void*>(pools_of_memory.back() + current_allocate_point);
        current_allocate_point += chunkSize;
        return memory_for_work;
    }
    void deallocate(void* point) {
        dealloc_memory.push(point);
    }
    ~FixedAllocator() {
        if (*how_many_ptrs <= 1) {
            for (int i = 0; i<int(pools_of_memory.size()); ++i) {
                delete[] pools_of_memory[i];
            }
            *how_many_ptrs -= 1;
            delete how_many_ptrs;
        }
        else {
            *how_many_ptrs -= 1;
        }

    }
};
template<size_t SZ>
bool operator==(const FixedAllocator<SZ>& all1, const FixedAllocator<SZ>& all2) {
    if (all1.pools_of_memory.back() == all2.pools_of_memory.back()) {
        return true;
    }
    else return false;
}


template <class T>
class FastAllocator {
private:
    FixedAllocator<24> fixedAlloc;
public:
    template<typename K>
    FastAllocator<T>(const FastAllocator<K>& all) {
        fixedAlloc = all.fixedAlloc;
    }
    FastAllocator& operator=(const FastAllocator& all) {
        fixedAlloc = all.fixedAlloc;
        return *this;
    }
    template<typename K>
    bool operator!=(const FastAllocator<K>& all) const {
        if (sizeof(K) != sizeof(T)) return true;
        else return false;
    }
    bool operator!=(const FastAllocator<T>& all) const {
        if (fixedAlloc == all.fixedAlloc) return false;
        else return true;
    }
    template<typename K>
    bool operator==(const FastAllocator<K>& all) const {
        if (sizeof(K) != sizeof(T)) return false;
        else return true;
    }
    bool operator==(const FastAllocator<T>& all) const {
        if (fixedAlloc == all.fixedAlloc) return true;
        else return false;
    }
    using value_type = T;
    
    FastAllocator() {}
    T* allocate(size_t n) {
        if (n * sizeof(T) <= 24) {
            return static_cast<T*>(fixedAlloc.allocate());
        }
        if (sizeof(T) == 1 || sizeof(T) == 8) {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        else {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
    }
    void deallocate(T* ptr, size_t n) {
        //  std::cout << sizeof(T) << " ";
        if (n * sizeof(T) <= 24) {
            fixedAlloc.deallocate(ptr);
        }
        else {
            operator delete(ptr);
        }
    }
};

template<typename T, typename Allocator = std::allocator<T>>
class List {

    struct Node {
        Node* prev = nullptr;
        Node* next = nullptr;
        T val;
        Node(const T& value) : val(value) {}
        Node() = default;
    };

    size_t sz = 0;
    Node* fake_node;

    typename std::allocator_traits<Allocator>::template rebind_alloc<Node> alloc;
    using AlTr = std::allocator_traits<typename std::allocator_traits<Allocator>::template rebind_alloc<Node>>;

    void push_back_without_argument() {
        //  std::cerr << "pub" << '\n';
        if (sz == 0) {
            Node* new_node = AlTr::allocate(alloc, 1);
            AlTr::construct(alloc, new_node);
            new_node->prev = fake_node;
            new_node->next = fake_node;
            fake_node->prev = new_node;
            fake_node->next = new_node;
        }
        else {
            Node* new_node = AlTr::allocate(alloc, 1);
            AlTr::construct(alloc, new_node);
            fake_node->prev->next = new_node;
            new_node->prev = fake_node->prev;
            fake_node->prev = new_node;
            new_node->next = fake_node;
        }
        sz++;
    }
public:
    explicit List(const Allocator& allocat = Allocator()) : alloc(allocat) {
        // std::cerr << "common_create\n";
        fake_node = AlTr::allocate(alloc, 1);
        fake_node->prev = fake_node;
        fake_node->next = fake_node;
    }
    List(size_t count, const T& value, const Allocator& allocat = Allocator()) : alloc(allocat) {
        //  std::cerr << "many_create\n";
        fake_node = AlTr::allocate(alloc, 1);
        while (this->size() != count) {
            this->push_back(value);
        }
    }
    List(size_t count, const Allocator& allocat = Allocator()) : alloc(allocat) {
        //  std::cerr << "many_create\n";
        fake_node = AlTr::allocate(alloc, 1);
        while (this->size() != count) {
            this->push_back_without_argument();
        }
    }
    //----------------------------------------------------------
    void push_back(const T& val) {
        //  std::cerr << "pub" << '\n';
        if (sz == 0) {
            Node* new_node = AlTr::allocate(alloc, 1);
            AlTr::construct(alloc, new_node, val);
            new_node->prev = fake_node;
            new_node->next = fake_node;
            fake_node->prev = new_node;
            fake_node->next = new_node;
        }
        else {
            Node* new_node = AlTr::allocate(alloc, 1);
            AlTr::construct(alloc, new_node, val);
            fake_node->prev->next = new_node;
            new_node->prev = fake_node->prev;
            fake_node->prev = new_node;
            new_node->next = fake_node;
        }
        sz++;
    }
    
    void push_front(const T& val) {
        //   std::cerr << "puf" << '\n';
        if (sz == 0) {
            Node* new_node = AlTr::allocate(alloc, 1);
            AlTr::construct(alloc, new_node, val);
            new_node->prev = fake_node;
            new_node->next = fake_node;
            fake_node->prev = new_node;
            fake_node->next = new_node;
        }
        else {
            Node* new_node = AlTr::allocate(alloc, 1);
            AlTr::construct(alloc, new_node, val);
            fake_node->next->prev = new_node;
            new_node->next = fake_node->next;
            fake_node->next = new_node;
            new_node->prev = fake_node;
        }
        sz++;
    }
    void pop_back() {
        //  std::cerr << "pob" << '\n';
        if (sz == 1) {
            AlTr::destroy(alloc, fake_node->prev);
            AlTr::deallocate(alloc, fake_node->prev, 1);
            fake_node->prev = fake_node;
            fake_node->next = fake_node;
        }
        else {
            Node* copy = fake_node->prev->prev;
            fake_node->prev->prev->next = fake_node;
            AlTr::destroy(alloc, fake_node->prev);
            AlTr::deallocate(alloc, fake_node->prev, 1);
            fake_node->prev = copy;
        }
        sz--;
    }
    void pop_front() {
        //    std::cerr << "pof" << '\n';
        if (sz == 1) {
            AlTr::destroy(alloc, fake_node->prev);
            AlTr::deallocate(alloc, fake_node->prev, 1);
            fake_node->prev = fake_node;
            fake_node->next = fake_node;
        }
        else {
            Node* copy = fake_node->next->next;
            fake_node->next->next->prev = fake_node;
            AlTr::destroy(alloc, fake_node->next);
            AlTr::deallocate(alloc, fake_node->next, 1);
            fake_node->next = copy;
        }
        sz--;
    }
    //-----------------------------------------------------------
    size_t size() const {
        return this->sz;
    }
    Allocator get_allocator() const {
        typename std::allocator_traits<Allocator>::template rebind_alloc<T> copy = alloc;
        return copy;
    }
    //-----------------------------------------------------------
    List(const List& l) {
        //      std::cerr << "create_from_list\n";
        alloc = AlTr::select_on_container_copy_construction(l.alloc);
        fake_node = AlTr::allocate(alloc, 1);
        fake_node->prev = fake_node;
        fake_node->next = fake_node;
        if (l.size() == 0) {

        }
        else {
            Node* temp = l.fake_node->next;
            for (size_t i = 0; i < l.size(); i++) {
                this->push_back(temp->val);
                temp = temp->next;
            }
        }
    }
    List& operator=(const List& l) {
        //   std::cerr << "operator_=\n";
        if (AlTr::propagate_on_container_copy_assignment::value && alloc != l.alloc) {
            alloc = l.alloc;
        }
        while (sz != 0) {
            this->pop_back();
        }
        Node* temp = l.fake_node->next;
        for (size_t i = 0; i < l.size(); i++) {
            this->push_back(temp->val);
            temp = temp->next;
        }
        return *this;
    }
    template<bool is_const >
    class common_iterator {
    public:
        std::conditional_t<is_const, const Node*, Node*> ref;

        using iterator = common_iterator<false>;
        using const_iterator = common_iterator<true>;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<is_const, const T*, T*>;
        using value_type = std::conditional_t<is_const, const T, T>;;
        using reference = std::conditional_t<is_const, const T&, T&>;

        common_iterator(Node* n) :ref(n) {
        }
        common_iterator(const common_iterator<false>& it) {
            ref = it.ref;
        }
        common_iterator& operator=(common_iterator it) {
            ref = it.ref;
            return *this;
        }

        //----------------------------------------------

        reference operator*() const {
            //    std::cerr << "*" << '\n';
            return ref->val;
        }
        pointer operator->() const {
            return &(ref->val);
        }

        //----------------------------------------------------

        common_iterator& operator++() {
            //   std::cerr << "++" << '\n';
            ref = ref->next;
            return *this;
        }
        common_iterator& operator--() {
            //  std::cerr << "--" << '\n';
            ref = ref->prev;
            return *this;
        }
        common_iterator operator++(int) {

            common_iterator copy = *this;
            copy.ref = copy.ref->next;
            ref = ref->next;
            return copy;
        }
        common_iterator operator--(int) {
            common_iterator copy = *this;
            copy.ref = copy.ref->prev;
            ref = ref->next;
            return copy;
        }

        //-----------------------------------------------------
        template<bool U>
        bool operator!=(common_iterator<U> it) const{
            if (ref == it.ref) return false;
            else return true;
        }
        template<bool U>
        bool operator==(common_iterator<U> it) const{
            if (ref == it.ref) return true;
            else return false;
        }

        ~common_iterator() = default;
    };

    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;



    //----------------------------------------------------------------------------

    iterator begin() {
        // std::cerr << "bgn\n";
        return common_iterator<false>(this->fake_node->next);
    }
    const_iterator begin() const {
        return common_iterator<true>(this->fake_node->next);
    }
    iterator end() {
        //  std::cerr << "end\n";
        return common_iterator<false>(this->fake_node);
    }
    const_iterator end() const {
        return common_iterator<true>(this->fake_node);
    }
    const_iterator cbegin() const {
        return common_iterator<true>(this->fake_node->next);
    }
    const_iterator cend() const {
        return common_iterator<true>(this->fake_node);
    }


    reverse_iterator rbegin() {
        iterator it(this->fake_node);
        return reverse_iterator(it);
    }
    const_reverse_iterator rbegin() const {
        const_iterator it(this->fake_node);
        return const_reverse_iterator(it);
    }
    reverse_iterator rend() {
        iterator it(this->fake_node->next);
        return reverse_iterator(it);
    }
    const_reverse_iterator rend() const {
        const_iterator it(this->fake_node->next);
        return const_reverse_iterator(it);
    }
    const_reverse_iterator crbegin() const {
        const_iterator it(this->fake_node);
        return const_reverse_iterator(it);
    }
    const_reverse_iterator crend() const {
        const_iterator it(this->fake_node->next);
        return const_reverse_iterator(it);
    }

    //---------------------------------------------

    void insert(iterator& it, const T& val = T()) {
        //   std::cerr << "in" << '\n';
        Node* new_node = AlTr::allocate(alloc, 1);
        AlTr::construct(alloc, new_node, val);
        new_node->prev = it.ref->prev;
        it.ref->prev->next = new_node;
        new_node->next = it.ref;
        it.ref->prev = new_node;
        sz++;
    }
    void insert(const_iterator& it, const T& val = T()) {
        //  std::cerr << "in" << '\n';
        Node* new_node = AlTr::allocate(alloc, 1);
        AlTr::construct(alloc, new_node, val);
        new_node->prev = const_cast<Node*>(it.ref->prev);
        it.ref->prev->next = new_node;
        new_node->next = const_cast<Node*>(it.ref);
        const_cast<Node*>(it.ref)->prev = new_node;
        sz++;
    }

    void erase(iterator it) {
        //  std::cerr << "er" << '\n';
        it.ref->prev->next = it.ref->next;
        it.ref->next->prev = it.ref->prev;
        AlTr::destroy(alloc, it.ref);
        AlTr::deallocate(alloc, it.ref, 1);
        sz--;
    }
    void erase(const_iterator it) {
        //   std::cerr << "er" << '\n';
        it.ref->prev->next = it.ref->next;
        it.ref->next->prev = it.ref->prev;
        AlTr::destroy(alloc, it.ref);
        AlTr::deallocate(alloc, const_cast<Node*>(it.ref), 1);
        sz--;
    }
    ~List() {
        //   std::cerr << "~\n";
        while (this->size() != 0) {
            this->pop_front();
        }
        AlTr::deallocate(alloc, fake_node, 1);
    }
};
