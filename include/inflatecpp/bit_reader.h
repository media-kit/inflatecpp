#ifndef _BIT_READER_H
#define _BIT_READER_H

#if defined(_M_X64) || defined(__x86_64__) || defined(__aarch64__)
#define X64BIT_SHIFTER
#endif /* defined(_M_X64) */

#ifdef X64BIT_SHIFTER
typedef unsigned long long shifter_t;
#else
typedef unsigned int shifter_t;
#endif /* X64BIT_SHIFTER */

class BitReader {
 public:
  BitReader();
  ~BitReader() = default;

  void Init(unsigned char*, unsigned char*);
  void ConsumeBits(const int);
  void ModifyInBlock(const int);
  void Refill32();

  unsigned int GetBits(const int);
  unsigned int PeekBits();

  int ByteAllign();

  unsigned char* GetInBlock() { return this->in_block_; };
  unsigned char* GetInBlockEnd() { return this->in_block_end_; };
  unsigned char* GetInBlockStart() { return this->in_block_start_; };

 private:
  int shifter_bit_count_;
  shifter_t shifter_data_;
  unsigned char* in_block_;
  unsigned char* in_block_end_;
  unsigned char* in_block_start_;
};

BitReader::BitReader() {
  this->shifter_bit_count_ = 0;
  this->shifter_data_ = 0;
  this->in_block_ = nullptr;
  this->in_block_end_ = nullptr;
  this->in_block_start_ = nullptr;
}

/**
 * Initialize bit reader
 *
 * @param in_block pointer to the start of the compressed block
 * @param in_block_end pointer to the end of the compressed block + 1
 */
void BitReader::Init(unsigned char* in_block, unsigned char* in_block_end) {
  this->in_block_ = in_block;
  this->in_block_end_ = in_block_end;
  this->in_block_start_ = in_block;
}

/** Refill 32 bits at a time if the architecture allows it, otherwise do
 * nothing. */
void BitReader::Refill32() {
#ifdef X64BIT_SHIFTER
  if (this->shifter_bit_count_ <= 32 &&
      (this->in_block_ + 4) <= this->in_block_end_) {
#if defined(_M_X64) || defined(__x86_64__)
    this->shifter_data_ |= (((shifter_t)(*((unsigned int*)this->in_block_)))
                            << this->shifter_bit_count_);
    this->shifter_bit_count_ += 32;
    this->in_block_ += 4;
#else
    this->shifter_data_ |=
        (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
    this->shifter_bit_count_ += 8;
    this->shifter_data_ |=
        (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
    this->shifter_bit_count_ += 8;
    this->shifter_data_ |=
        (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
    this->shifter_bit_count_ += 8;
    this->shifter_data_ |=
        (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
    this->shifter_bit_count_ += 8;
#endif
  }
#endif /* X64BIT_SHIFTER */
}

/**
 * Consume variable bit-length value, after reading it with PeekBits()
 *
 * @param n size of value to consume, in bits
 */
void BitReader::ConsumeBits(const int n) {
  this->shifter_data_ >>= n;
  this->shifter_bit_count_ -= n;
}

/**
 * Read variable bit-length value
 *
 * @param n size of value in bits (number of bits to read), 0..16
 *
 * @return value, or -1 for failure
 */
unsigned int BitReader::GetBits(const int n) {
  if (this->shifter_bit_count_ < n) {
    if (this->in_block_ < this->in_block_end_) {
      this->shifter_data_ |=
          (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
      this->shifter_bit_count_ += 8;

      if (this->in_block_ < this->in_block_end_) {
        this->shifter_data_ |=
            (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
        this->shifter_bit_count_ += 8;
      }
    } else
      return -1;
  }

  unsigned int value = this->shifter_data_ & ((1 << n) - 1);
  this->shifter_data_ >>= n;
  this->shifter_bit_count_ -= n;
  return value;
}

/**
 * Peek at a 16-bit value in the bitstream (lookahead)
 *
 * @return value
 */
unsigned int BitReader::PeekBits() {
  if (this->shifter_bit_count_ < 16) {
    if (this->in_block_ < this->in_block_end_) {
      this->shifter_data_ |=
          (((shifter_t)(*this->in_block_++)) << this->shifter_bit_count_);
      if (this->in_block_ < this->in_block_end_)
        this->shifter_data_ |= (((shifter_t)(*this->in_block_++))
                                << (this->shifter_bit_count_ + 8));
      this->shifter_bit_count_ += 16;
    }
  }

  return this->shifter_data_ & 0xffff;
}

/** Re-align bitstream on a byte */
int BitReader::ByteAllign() {
  while (this->shifter_bit_count_ >= 8) {
    this->shifter_bit_count_ -= 8;
    this->in_block_--;
    if (this->in_block_ < this->in_block_start_) return -1;
  }

  this->shifter_bit_count_ = 0;
  this->shifter_data_ = 0;
  return 0;
}

void BitReader::ModifyInBlock(const int v) { this->in_block_ += v; }

#endif /* !_BIT_READER_H */