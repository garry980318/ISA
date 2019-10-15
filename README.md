# ISA
Napište program dns, který bude umět zasílat dotazy na DNS servery a v čitelné podobě vypisovat přijaté odpovědi na standardní výstup. Sestavení a analýza DNS paketů musí být implementována přímo v programu dns. Stačí uvažovat pouze komunikaci pomocí UDP.

Při vytváření programu je povoleno použít hlavičkové soubory pro práci se sokety a další obvyklé funkce používané v síťovém prostředí (jako je netinet/*, sys/*, arpa/* apod.), knihovnu pro práci s vlákny (pthread), signály, časem, stejně jako standardní knihovnu jazyka C (varianty ISO/ANSI i POSIX), C++ a STL. Jiné knihovny nejsou povoleny.

Spuštění aplikace
Použití: dns [-r] [-x] [-6] -s server [-p port] adresa

Pořadí parametrů je libovolné. Popis parametrů:
* -r: Požadována rekurze (Recursion Desired = 1), jinak bez rekurze.
* -x: Reverzní dotaz místo přímého.
* -6: Dotaz typu AAAA místo výchozího A.
* -s: IP adresa nebo doménové jméno serveru, kam se má zaslat dotaz.
* -p port: Číslo portu, na který se má poslat dotaz, výchozí 53.
* adresa: Dotazovaná adresa.
