cd -- "$(dirname "$BASH_SOURCE")"

python3 ../test/external-utils/HppAmalgamator/amalgamate_header_files.py ../source/AudioTraits.hpp ../single_include/AudioTraits.hpp
