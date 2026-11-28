#include <stdio.h>
#include "update.h"

static int fails;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "update_test: FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    char tag[64], url[1024], dig[80];
    static const char json[] =
        "{\n \"tag_name\": \"v0.2.2\",\n \"assets\": [\n"
        "  {\"name\": \"nso-0.2.2-src.tar.gz\", \"digest\": \"sha256:"
        "1111111111111111111111111111111111111111111111111111111111111111\","
        "   \"browser_download_url\": \"https://github.com/Vulto/nso/releases/download/v0.2.2/nso-0.2.2-src.tar.gz\"},\n"
        "  {\"name\": \"nso-0.2.2-linux-x86_64.tar.gz\", \"digest\": \"sha256:"
        "abcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcd\","
        "   \"browser_download_url\": \"https://github.com/Vulto/nso/releases/download/v0.2.2/nso-0.2.2-linux-x86_64.tar.gz\"}\n"
        " ]\n}";

    CHECK(Update_VersionNewer("v0.2.2", "0.2.1"));
    CHECK(Update_VersionNewer("0.10.0", "0.9.9"));
    CHECK(Update_VersionNewer("v1.0", "0.9.9"));
    CHECK(!Update_VersionNewer("v0.2.1", "0.2.1"));
    CHECK(!Update_VersionNewer("v0.2.0", "0.2.1"));
    CHECK(!Update_VersionNewer("nightly", "0.2.1"));
    CHECK(!Update_VersionNewer("", "0.2.1"));

    CHECK(Update_ParseRelease(json, tag, sizeof tag, url, sizeof url, dig, sizeof dig, 0));
    CHECK(strcmp(tag, "v0.2.2") == 0);
    CHECK(strstr(url, "linux-x86_64.tar.gz") != NULL);
    CHECK(strcmp(dig, "abcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcd") == 0);

    /* foreign host is refused unless the test override is on */
    {
        static const char evil[] =
            "{\"tag_name\":\"v9.9.9\",\"assets\":[{\"browser_download_url\":"
            "\"https://evil.example/nso-9.9.9-linux-x86_64.tar.gz\"}]}";
        CHECK(!Update_ParseRelease(evil, tag, sizeof tag, url, sizeof url, dig, sizeof dig, 0));
        CHECK(Update_ParseRelease(evil, tag, sizeof tag, url, sizeof url, dig, sizeof dig, 1));
        CHECK(dig[0] == '\0');
    }
    /* no linux asset, bad tag */
    CHECK(!Update_ParseRelease("{\"tag_name\":\"v1.0.0\",\"assets\":[]}", tag, sizeof tag, url, sizeof url, dig, sizeof dig, 0));
    CHECK(!Update_ParseRelease("{\"tag_name\":\"v1;rm\",\"assets\":[]}", tag, sizeof tag, url, sizeof url, dig, sizeof dig, 1));
    CHECK(!Update_ParseRelease("not json", tag, sizeof tag, url, sizeof url, dig, sizeof dig, 1));

    if (!fails) puts("update_test: ok");
    return fails ? 1 : 0;
}
