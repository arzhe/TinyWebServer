#ifndef WEB_SERVER_BUFFER_BUFFER_H
#define WEB_SERVER_BUFFER_BUFFER_H

#include <cstring>
#include <iostream>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <assert.h>

/***************************************************
 * Buffer structure
 *
 * +----------------+-------------+-------------+
 * |PrependableBytes|ReadableBytes|WritableBytes|
 * +----------------+-------------+-------------+
 * 0                1             2
 * |                |             |
 * +--------------+ |             |
 *                | |             |
 * 0 : BeginPtr <-+ |             |
 * 1 : read_pos <---+             |
 * 2 : write_pos <----------------+
 *
 ****************************************************/

class Buffer {
private:
    std::vector<char> buffer;
    std::atomic<std::size_t> read_pos;
    std::atomic<std::size_t> write_pos;

    char* BeginPtr();
    const char* BeginPtr() const;

    // Reserves "len" bytes of space in the Buffer. 
    void MakeSpace(size_t len);

public:
    Buffer(int init_buffer_size = 1024);
    ~Buffer() = default;

    /*
     * WritableBytes returns writable bytes.
     *
     * ReadableBytes returns readable bytes.
     *
     * PrependableBytes returns prependable bytes.
     */
    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;
    
    // Returns a pointer to read_pos.
    const char* Peek() const;

    // Ensures there is enough space for writing data into Buffer.
    void EnsureWriteable(size_t len);

    // Updates "len" bytes for write_pos.
    void HasWritten(size_t len);

    // Updates "len" bytes for read_pos.
    void Retrieve(size_t len);

    // Updates "end - Peek()" bytes for read_pos.
    void RetrieveUntil(const char* end);

    // Retrieves all readable data in Buffer, that is to say, "clears" Buffer.
    void RetrieveAll();

    // Retrieves the rest data in readable section and returns them in std::string type.
    std::string RetrieveAllToStr();
    
    // Returns a pointer to write_pos.
    const char* BeginWriteConst() const;
    char* BeginWrite();

    // Appends Buffer.
    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buff);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);
};

#endif
