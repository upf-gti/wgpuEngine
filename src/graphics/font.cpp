#include "font.h"

using namespace std::string_literals;

float Font::adjust_kerning_pairs(int first, int second) {

    typedef std::multimap<int, CKerning>::iterator kiterator;
    if (!m_kernings.count(first))
        return 0;

    std::pair<kiterator, kiterator> result = m_kernings.equal_range(first);

    for (kiterator it = result.first; it != result.second; ++it)
    {
        CKerning& k = it->second;
        if (k.second == second) {
            return (float)k.amount;
        }
    }

    return 0;
}