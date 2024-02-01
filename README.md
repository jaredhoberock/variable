# variable

`variable<"name">` is a named placeholder for a value which is yet unknown.

For example, `variable<"block_size">` could be a placeholder in an arithmetic expression which encodes a value dependent on the block size of a CUDA kernel launch before the shape of the launch has been decided:

    std::size_t n = ...

    variable<"block_size"> block_size;
    auto num_blocks = ceil_div(block_size, n);
    std::tuple unevaluated_config(block_size, num_blocks);

    // unevaluated_config is an unevaluated expression of a variable "block_size"

    ...

    // later, evaluate the launch configuration after choosing a block size
    int actual_block_size = choose_kernel_block_size();
    
    std::tuple<int,int> config = evaluate(unevaluated_config, {"block_size", actual_block_size});

A user-defined literal operator is included to make it convenient to spell these constants.

    // these two spellings are equivalent:
    variable<"block_size"> block_size_1;
    auto block_size_2 = "block_size"_v;
