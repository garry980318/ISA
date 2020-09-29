# ISA - Discord bot

Vytvořte program isabot, který bude působit jako bot na komunikační službě Discord. Bot se připojí na Discord server na kanál "#isa-bot" a bude reagovat na všechny zprávy zaslané ostatními uživateli. Bot bude fungovat jako echo pro všechny zprávy, které zachytí. V případě, že bot na daném kanále zachytí jakoukoli zprávu jiného uživatele (tedy jinou než svou vlastní) a zároveň, která není jiného bota (uživatelské jméno neobsahuje podřetězec "bot"), odešle tuto zprávu zpátky na kanál a to ve formátu `echo: <username> - <message>` (kde `<username>` představuje uživatelské jméno uživatele, který odeslal původní zprávu).

Kdekoli v zadání, kde uvidíte <nejaky_label>, uvažujte vždy výstup bez znaků "<" a ">".

Při vytváření programu je povoleno použít pouze hlavičkové soubory pro práci se sokety a další obvyklé funkce používané v síťovém prostředí (jako je netinet/_, sys/_, arpa/_, openssl/_ apod.), knihovnu pro práci s vlákny (pthread), signály, časem, stejně jako standardní knihovnu jazyka C (varianty ISO/ANSI i POSIX), C++ a STL. Jiné knihovny nejsou povoleny.

## Spuštění programu

Použití: `isabot [-h|--help] [-v|--verbose] -t <bot_access_token>`

Pořadí parametrů je libovolné. Popis parametrů:

-   Spuštění programu bez parametrů zobrazí nápovědu.
-   `-h|--help` : Vypíše nápovědu na standardní výstup.
-   `-v|--verbose` : Bude zobrazovat zprávy, na které bot reaguje na standardní výstup ve formátu `<channel> - <username>: <message>"`.
-   `-t <bot_access_token>` : Zde je nutno zadat autentizační token pro přístup bota na Discord.

Ukončení programu proběhne zasláním signálu SIGINT (tedy například pomocí kombinace kláves Ctrl + c), do té doby bude bot vykonávat svou funkcionalitu.
