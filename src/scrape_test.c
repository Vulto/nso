#define SCRAPE_IMPLEMENTATION
#include "scrape.h"

#include <stdio.h>
#include <string.h>

static int fails = 0;

static void expect_match(const char *title, const char **names, int n, const char *want)
{
    const char *got = Scrape_Match(title, names, n);
    if ((want == NULL && got != NULL) || (want && (got == NULL || strcmp(got, want) != 0))) {
        fprintf(stderr, "match %s -> %s, want %s\n", title,
                got ? got : "(null)", want ? want : "(null)");
        fails++;
    }
}

int main(void)
{
    const char *names[] = {
        "Super Mario Bros. 3 (USA).png",
        "Super Mario Bros. (Japan).png",
        "Super Mario Bros. (USA).png",
        "Legend of Zelda, The - Twilight Princess (Japan).png",
        "Legend of Zelda, The - Twilight Princess (USA).png",
        "Dr. Mario 64 (Japan).png",
        "Dr. Mario 64 (USA).png",
    };
    char repo[64];
    char **parsed = NULL;
    int count = 0;
    const char *json =
        "{\"path\": \"Named_Boxarts\"}"
        "{\"path\": \"Named_Boxarts/readme.txt\"}"
        "{\"path\": \"Named_Boxarts/Super Mario Bros. (USA).png\"}"
        "{\"path\":\"Named_Boxarts/Dr. Mario 64 (USA).png\"}";

    expect_match("Super Mario Bros", names, 7, "Super Mario Bros. (USA).png");
    expect_match("Super Mario Bros. 3", names, 7, "Super Mario Bros. 3 (USA).png");
    expect_match("Twilight Princess", names, 7,
                 "Legend of Zelda, The - Twilight Princess (USA).png");
    expect_match("Dr. Mario 64", names, 7, "Dr. Mario 64 (USA).png");

    {
        const char *snes[] = {
            "Super Mario World (Europe).png",
            "Super Mario World (USA).png",
            "Super Metroid (Japan, USA) (En,Ja).png",
            "Super Metroid (USA).png",
            "Legend of Zelda, The - A Link to the Past (Japan).png",
            "Legend of Zelda, The - A Link to the Past (USA).png",
            "Legend of Zelda, The - Twilight Princess (USA).png",
            "Kirby Super Star (USA).png",
        };
        expect_match("The Legend of Zelda: A Link to the Past", snes, 8,
                     "Legend of Zelda, The - A Link to the Past (USA).png");
        expect_match("Super Mario World", snes, 8, "Super Mario World (USA).png");
        expect_match("Super Metroid", snes, 8, "Super Metroid (USA).png");
        expect_match("The Legend of Zelda: Oracle of Ages", snes, 8, NULL);
    }

    Scrape_Repo(repo, sizeof(repo), "Nintendo - Nintendo 64");
    if (strcmp(repo, "Nintendo_-_Nintendo_64") != 0) {
        fprintf(stderr, "repo %s\n", repo);
        fails++;
    }
    {
        char url[1024];
        const char *want =
            "https://thumbnails.libretro.com/Nintendo%20-%20Nintendo%2064/"
            "Named_Boxarts/Super%20Mario%2064%20%28USA%29.png";
        if (Scrape_ImageUrl(url, sizeof(url), "Nintendo - Nintendo 64",
                            "Super Mario 64 (USA).png") != 0
            || strcmp(url, want) != 0) {
            fprintf(stderr, "url %s\n", url);
            fails++;
        }
    }

    if (Scrape_ParseIndex(json, &parsed, &count) != 0 || count != 2
        || strcmp(parsed[0], "Super Mario Bros. (USA).png") != 0
        || strcmp(parsed[1], "Dr. Mario 64 (USA).png") != 0) {
        fprintf(stderr, "index count %d\n", count);
        fails++;
    }
    Scrape_FreeNames(parsed, count);

    parsed = NULL;
    count = 0;
    if (Scrape_ParseIndex("{\"truncated\": true}", &parsed, &count) == 0) {
        fprintf(stderr, "truncated index was accepted\n");
        fails++;
    }
    Scrape_FreeNames(parsed, count);

    return fails ? 1 : 0;
}
