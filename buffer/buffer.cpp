#include <sys/uio.h>

#include "./buffer.h"

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

Buffer::Buffer(int initBuffSize) : buffer(initBuffSize), read_pos(0), write_pos(0) {}

size_t Buffer::ReadableBytes() const {
    return write_pos - read_pos;
}

size_t Buffer::WritableBytes() const {
    return buffer.size() - write_pos;
}

size_t Buffer::PrependableBytes() const {
    return read_pos;
}

const char* Buffer::Peek() const {
    return BeginPtr() + read_pos;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    read_pos += len;
}

void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll() {
    bzero(&buffer[0], buffer.size());
    read_pos = 0;
    write_pos = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr() + write_pos;
}

char* Buffer::BeginWrite() {
    return BeginPtr() + write_pos;
}

void Buffer::HasWritten(size_t len) {
    write_pos += len;
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

void Buffer::EnsureWriteable(size_t len) {
    if(WritableBytes() < len) {
        MakeSpace(len);
    }

    assert(WritableBytes() >= len);
}

char* Buffer::BeginPtr() {
    return &(*(buffer.begin()));
}

const char* Buffer::BeginPtr() const {
    return &(*(buffer.begin()));
}

void Buffer::MakeSpace(size_t len) {
    if(WritableBytes() + PrependableBytes() < len) {
        buffer.resize(write_pos + len + 1);
    }
    else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr() + read_pos, BeginPtr() + write_pos, BeginPtr());
        read_pos = 0;
        write_pos = read_pos + readable;
        assert(readable == ReadableBytes());
    }
}

ssize_t Buffer::ReadFd(int fd, int* saveErrno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    
    iov[0].iov_base = BeginPtr() + write_pos;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(len) <= writable) {
        write_pos += len;
    }
    else {
        write_pos = buffer.size();
        Append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int* saveErrno) {
    size_t read_size = ReadableBytes();
    ssize_t len = write(fd, Peek(), read_size);
    if(len < 0) {
        *saveErrno = errno;
        return len;
    }
    read_pos += len;
    return len;
}
