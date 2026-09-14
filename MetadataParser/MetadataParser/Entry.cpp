#include "Global.h"

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("Usage: MetadataParser.exe <config.txt> <global-metadata.dat> [--type-only] [--verbose]\n");
        return EXIT_FAILURE;
    }

    for (int i = 3; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "--type-only")
            Metadata::SetTypeOnly(true);
        else if (arg == "--verbose")
            Metadata::SetVerbose(true);
        else
            printf("Unknown option ignored: %s\n", argv[i]);
    }

    printf("Reading config...\n");
    if (!Config::Read(argv[1]))
    {
        printf("Failed to read config\n");
        return EXIT_FAILURE;
    }

    printf("Reading metadata...\n");
    std::vector<uint8_t> buffer;
    if (!Utils::ReadFile(argv[2], buffer))
    {
        printf("Failed to read metadata\n");
        return EXIT_FAILURE;
    }

    printf("Processing metadata...\n");
    if (!Metadata::Process(buffer))
    {
        printf("Failed to process metadata\n");
        return EXIT_FAILURE;
    }

    printf("Writing metadata...\n");
    if (!Utils::WriteFile(std::string(argv[2]) + ".protec", buffer))
    {
        printf("Failed to write metadata\n");
        return EXIT_FAILURE;
    }

    printf("Done. Output: %s.protec\n", argv[2]);
    return EXIT_SUCCESS;
}
