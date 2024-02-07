A threaded, double-buffered file reader library in C++.

It's useful for chugging through large files in programs where the main thread
is processing the file linearly from beginning to end. It would generally not
be useful in cases where the file is small enough to fit in memory, or the
processing of the file requires seeking.

```c++
BufferedReader reader;
if (!reader.open("file.txt"))
    printf("Open failed\n");
```

The `open` method takes an optional parameter, `size_mb`:

```c++
reader.open("file.txt", 1024);
```

In this case, it would use 1024 MB total (512 MB per buffer). The default size
is 256 MB (128 MB per buffer).

As soon as `open` is called, it kicks off the first read on a background
thread. The next buffer can be accessed with the `swap` method:

```c++
size_t bytes_read;
uint8_t *buffer = reader.swap(bytes_read);
```

The thread will block if data isn't yet available. As soon as `swap` returns,
it kicks off the next read if the previous read succeeded and there is still
data left.

In this way, the program can process the current buffer while the back buffer
is filled with the next chunk of the file in the background.

If `swap` returns `nullptr`, this indicates an error. If `swap` succeeds and
`bytes_read` is 0, this indicates that the end of the file has been reached.

When done with the reader, you can call `close`:

```c++
reader.close();
```

This isn't strictly necessary, since `close` will be called by the reader's
destructor. However, it can be used to clean up a reader before reaching the
end of the file. The `close` method will block until the current read finishes.
