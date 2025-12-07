// native/patches/juce_coreaudio_stride_iterator_traits.h
#pragma once

#include <iterator>
#include <type_traits>

// Forward-declare the JUCE type we want to specialise
namespace juce {
namespace CoreAudioClasses {
namespace CoreAudioInternal {

template <typename Ptr>
struct StrideIterator;

        } // namespace CoreAudioInternal
    } // namespace CoreAudioClasses
} // namespace juce

// Extend std::iterator_traits for JUCE's StrideIterator so libc++ algorithms are happy
namespace std {

    template <typename Ptr>
    struct iterator_traits<juce::CoreAudioClasses::CoreAudioInternal::StrideIterator<Ptr>>
    {
        using difference_type   = std::ptrdiff_t;

        // Ptr is typically something like `const float*`
        using value_type        = std::remove_cv_t<std::remove_pointer_t<Ptr>>;
        using pointer           = Ptr;
        using reference         = value_type&;
        using iterator_category = std::random_access_iterator_tag;
    };

} // namespace std
