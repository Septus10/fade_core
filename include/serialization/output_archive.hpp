#ifndef FADE_CORE_SERIALIZATION_OUTPUT_ARCHIVE_HPP_
#define FADE_CORE_SERIALIZATION_OUTPUT_ARCHIVE_HPP_

namespace fade {

class OutputArchive
{

};

template <typename T>
concept OutputArchiveType = std::derived_from<T, OutputArchive>;

}

#endif