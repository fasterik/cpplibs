A simple hash table in C++ that uses open addressing and linear probing.

Currently it only supports integers as keys and trivially copyable values.

Set a value:

```c++
HashTable<uint64_t, MyData> table;
MyData data = create_data();
table.set(42, data);
```

Use `emplace` to add the key and get a reference to uninitialized memory (or
the current value if the key is already present). This is preferable to
initializing a value on the stack and copying it.

```c++
MyData &value = table.emplace(42);
value = create_data();
```

Get a value from the table (returns `nullptr` if the key is not present):

```c++
MyData *result = table.get(42);
if (result)
    printf("Value is present\n");
```

Remove a key from the table (does nothing if the key is not present):

```c++
table.remove(42);
```

Clear the table:

```c++
table.clear();
```

Get the number of items in the table:

```c++
size_t count = table.count();
```

During testing I found that this implementation is 33% faster than
`unordered_map` doing random insertions and deletions. However, I still need to
write a realistic benchmark.
