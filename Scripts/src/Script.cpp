#include "Scripts/Script.h"
#include "Scripts/NativeScript.h"
#include "Scripts/Player.h"

#include <iostream>

EXPORT NativeScript* _cdecl CreateNativeScript()
{
    return new Player;
}

void Player::Start()
{
    std::cout << "Start from player script\n";
}

void Player::Update()
{
    std::cout << "Update\n";
}
