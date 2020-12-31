#include "Scripts/Script.h"
#include "Scripts/NativeScript.h"
#include "Scripts/Player.h"

#include <iostream>

NATIVE_SCRIPT_API NativeScript* _cdecl CreateNativeScript()
{
    return new Player;
}

void Player::Start()
{
    std::cout << "Start50\n";
}

void Player::Update()
{
    std::cout << "Update\n";
}
