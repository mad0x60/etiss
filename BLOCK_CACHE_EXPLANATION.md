# Understanding ETISS Block Cache: Fast Path vs Slow Path

## The Problem

ETISS translates instructions into blocks of compiled code. When executing, it needs to find the right block quickly. There are two ways to find a block:

1. **Fast Path**: Follow pre-linked pointers (`next`/`branch`)
2. **Slow Path**: Search through a hash table (`blockmap_`)

## BlockLink Structure

Each compiled block has this structure:

```cpp
class BlockLink {
    uint64_t start;        // Start address of this block
    uint64_t end;          // End address of this block
    BlockLink *next;       // Pointer to next sequential block
    BlockLink *branch;     // Pointer to branch target block
    ExecBlockCall execBlock; // Function pointer to compiled code
    // ...
};
```

## Example Execution Flow

Imagine this simple program:

```c
// Block A: addresses 0x1000-0x1010
int x = 10;
if (x > 5) {
    // Block B: addresses 0x1010-0x1020
    x = 20;
} else {
    // Block C: addresses 0x1020-0x1030
    x = 30;
}
// Block D: addresses 0x1030-0x1040
return x;
```

### Initial State (No Links)

When blocks are first compiled, they're stored in `blockmap_` but NOT linked:

```
blockmap_[...] = [BlockA, BlockB, BlockC, BlockD]
                  (all blocks exist, but no pointers between them)
```

### Execution Flow

**Step 1: Execute Block A (address 0x1000)**
- `getBlockFast(prev=nullptr, addr=0x1000)`
- No previous block, so can't use fast path
- Calls `getBlock()` → searches `blockmap_` → finds BlockA
- **Cache miss** (counted) but found in blockmap (no compilation needed)

**Step 2: Block A branches to Block B (address 0x1010)**
- `getBlockFast(prev=BlockA, addr=0x1010)`
- Checks `BlockA->next` → NULL (not linked yet)
- Checks `BlockA->branch` → NULL (not linked yet)
- **Cache miss** (counted)
- Calls `getBlock()` → searches `blockmap_` → finds BlockB
- **Links BlockA->branch = BlockB** (line 342 in Translation.cpp)

**Step 3: Block B continues to Block D (address 0x1030)**
- `getBlockFast(prev=BlockB, addr=0x1030)`
- Checks `BlockB->next` → NULL
- Checks `BlockB->branch` → NULL
- **Cache miss** (counted)
- Calls `getBlock()` → finds BlockD in blockmap
- **Links BlockB->next = BlockD** (line 338)

**Step 4: Next iteration - Block A branches to Block B again**
- `getBlockFast(prev=BlockA, addr=0x1010)`
- Checks `BlockA->branch` → **BlockB** (already linked!)
- **Cache HIT (branch)** ✅
- Returns immediately without searching blockmap

**Step 5: Block B continues to Block D again**
- `getBlockFast(prev=BlockB, addr=0x1030)`
- Checks `BlockB->next` → **BlockD** (already linked!)
- **Cache HIT (next)** ✅
- Returns immediately

## Visual Representation

### After First Execution (Links Established)

```
BlockA (0x1000-0x1010)
  ├─ next: NULL
  └─ branch: ──────┐
                    │
                    ▼
                BlockB (0x1010-0x1020)
                  ├─ next: ──────┐
                  └─ branch: NULL│
                                 │
                                 ▼
                             BlockD (0x1030-0x1040)
```

### Fast Path vs Slow Path

**Fast Path (getBlockFast):**
```cpp
// Check if next block is already linked
if (prev->next != NULL && matches address) {
    return prev->next;  // ✅ FAST - direct pointer access
}

// Check if branch target is already linked
if (prev->branch != NULL && matches address) {
    return prev->branch;  // ✅ FAST - direct pointer access
}

// Not found via pointers → cache miss
miss_count_++;
return getBlock(...);  // ⚠️ SLOW - search blockmap
```

**Slow Path (getBlock):**
```cpp
// Search through blockmap hash table
for (block in blockmap_[address >> 9]) {
    if (block covers address) {
        // Found! Link it for next time
        if (sequential) prev->next = block;
        else prev->branch = block;
        return block;  // ✅ Found, but slower
    }
}

// Not found → compile new block
compile_block();
add_to_blockmap();
link_to_prev();
return new_block;
```

## Why Cache Misses > Blocks Compiled?

In your example:
- **57 blocks compiled** (unique blocks)
- **323 cache misses** (fast path failures)

This happens because:

1. **First time accessing a block**: Cache miss, but found in blockmap (no compilation)
2. **Before linking**: Multiple cache misses until link is established
3. **Control flow**: Many branches mean many blocks aren't linked immediately

### Example Timeline

```
Time 1: Execute BlockA → miss → find in blockmap → link BlockA->branch = BlockB
Time 2: Execute BlockB → miss → find in blockmap → link BlockB->next = BlockD
Time 3: Execute BlockA → HIT via branch! ✅
Time 4: Execute BlockB → HIT via next! ✅
Time 5: Execute BlockC (else branch) → miss → find in blockmap → link BlockA->branch = BlockC
Time 6: Execute BlockC → miss → find in blockmap → link BlockC->next = BlockD
Time 7: Execute BlockA → HIT via branch! ✅ (now points to BlockC)
```

## Summary

- **Fast Path**: Follow `next`/`branch` pointers (O(1), very fast)
- **Slow Path**: Search `blockmap_` hash table (O(n) where n = blocks in bucket)
- **Cache Miss**: Fast path failed, had to use slow path
- **Linking**: When slow path finds a block, it links it for future fast access
- **Cache Misses > Compilations**: Many misses are resolved from blockmap without recompiling

The linking happens dynamically as execution progresses, building a graph of frequently-taken paths.

