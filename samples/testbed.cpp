#include "fixie.h"

int main(int argc, char** argv)
{
    fixie_context ctx = fixie_create_context();

    fixie_destroy_context(ctx);
}