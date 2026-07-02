# AionClientLauncher

Launcher simples em C++/Visual Studio para iniciar o cliente correto do Aion.

## Como usar

1. Abra `AionClientLauncher.sln` no Visual Studio.
2. Compile em `Release | x86`.
3. Copie `build\x86\Release\AionLauncher.exe` para a raiz do cliente, ao lado de `bin32`, `bin64`, `version.ini` e `aion.ico`.
4. Use `AionLauncher.exe` para abrir o jogo.

O launcher detecta se o Windows do usuario e 64-bit. Se for, abre `bin64\aion.exe`; caso contrario abre `bin32\aion.exe`. Se a opcao preferida estiver ausente, ele tenta a outra.

Qualquer parametro passado para `AionLauncher.exe` e repassado para o `aion.exe` escolhido.
