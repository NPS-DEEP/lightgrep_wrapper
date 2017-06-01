## How lightgrep callback works

### Setting up the scanner callback

Put your scanner's callback function into lightgrep's `void*` pointer in the pattern table:

    lg_pattern_info(pattern_map, i)->UserData = const_cast<void*>(static_cast<const void*>(callback_ptr));

* `i` is the index of the pattern, starting at 0.
* `pattern_map` is the handle created by, for example, `lg_create_pattern_map(1000)`.

Instead of using `lg_pattern_info` we could have a table of just callbacks indexed by KeywordIndex, for example `callbacks[KeywordIndex]`.

### Scanning
The `lg_search` family takes parameters including:

* `void* user_data`
* `LG_HITCALLBACK_FN callback`

When search finds something, it calls `LG_HITCALLBACK_FN` and provides user data.  The callback function takes two parameters: user data and `LG_SearchHit*`:

    // Basic Search Hit struct
    
    typedef struct {
      uint64_t  Start,        // starting offset of the hit
                End;          // one past the hit, i.e., End - Start = Length
      uint32_t  KeywordIndex; // index of keyword that hit
    } LG_SearchHit;

    // function you specify to handle the search hit, e.g.,
    // void gotASearchHit(void* userData, const LG_SearchHit const* hit) {
    //   print("hit at %d, ending %d, on keyword %d",
    //         hit->Start, hit->End, hit->KeywordIndex);
    // }
    typedef void (*LG_HITCALLBACK_FN)(void* user_data, const LG_SearchHit* const hit);

### The callback chain

When a search hit match is found, lightgrep calls `LG_HITCALLBACK_FN`.
From there, it calls the callback for the particular scanner indexed by KeywordIndex available in `LG_SearchHit` using `lg_pattern_info`.

#### Use `void* user_data` to call back

* Set up `user_data` to include the callback lookup handle.
* Obtain `KeywordIndex` from the `LG_SearchHit` structure.
* Call back to the indexed function.

#### `LG_HITCALLBACK_FN`
Generic entry to service the hit.

    void service_hit(void* user_data, const LG_SearchHit* hit) {
      HitData* hit_data(static_cast<HitData*>(user_data));

    ...

    }

