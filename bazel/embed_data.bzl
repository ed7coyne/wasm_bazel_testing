def cc_embed_data(name, src, variable_name, header_name = None):
    """Generates a C++ header containing the contents of a file as a string literal.

    Args:
        name: The name of the target.
        src: The source file to embed.
        variable_name: The name of the C++ variable to create.
        header_name: Optional name for the output header file. Defaults to <name>.h.
    """
    if not header_name:
        header_name = name + ".h"
    
    # We use a raw string literal R"()". Note that if the source file contains )", this will fail.
    # For Flatbuffers schemas, this is generally safe.
    native.genrule(
        name = name,
        srcs = [src],
        outs = [header_name],
        cmd = "echo 'static const char* " + variable_name + " = R\"(' > $@ && cat $< >> $@ && echo ')\";' >> $@",
    )
