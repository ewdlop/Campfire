#include "Scripts/NativeScript.h"

class Player : public NativeScript
{
public:
    virtual void Start() override;

    virtual void Update() override;
};
