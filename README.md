# AionClientLauncher

Launcher simples em C++/Visual Studio para iniciar o cliente correto do Aion.

## Distribuicao recomendada

Compile e distribua o launcher em `Release | x86` / `Win32`.

Esse unico executavel roda em Windows 10/11 32-bit e tambem em Windows 10/11
64-bit. Mesmo sendo x86, ele detecta quando o Windows nativo e 64-bit e abre
`bin64\aion.exe`; em Windows 32-bit ele abre `bin32\aion.exe`.

O projeto esta configurado com runtime C++ estatico (`/MT`) em Release. Assim o
`AionLauncher.exe` nao depende do .NET SDK nem do Microsoft Visual C++
Redistributable instalado no computador do jogador.

Importante: isso remove as dependencias do launcher. O `aion.exe` ainda pode ter
dependencias proprias. Se o cliente exigir Microsoft Visual C++ 2013, distribua
os instaladores oficiais junto do jogo e instale:

- `vcredist_x86.exe` para `bin32\aion.exe`
- `vcredist_x64.exe` para `bin64\aion.exe`

Em Windows 64-bit, o ideal e ter os dois redistributables se o pacote contem
`bin32` e `bin64`.

## Como usar

1. Abra `AionClientLauncher.sln` no Visual Studio.
2. Compile em `Release | x86`.
3. Copie `build\AionLauncher.exe` para a raiz do cliente, ao lado de `bin32`, `bin64`, `version.ini` e `aion.ico`.
4. Use `AionLauncher.exe` para abrir o jogo.

Ao compilar `Release | x86`, o projeto tambem gera
`build\x86\Release\AionLauncher.exe` e copia automaticamente esse executavel
para `build\AionLauncher.exe`.

O launcher detecta se o Windows do usuario e 64-bit. Se for, abre `bin64\aion.exe`; caso contrario abre `bin32\aion.exe`. Se a opcao preferida estiver ausente, ele tenta a outra.

Qualquer parametro passado para `AionLauncher.exe` e repassado para o `aion.exe` escolhido.
