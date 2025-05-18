#ifndef MACROS_H
#define MACROS_H

#define DELAY(delaySeconds, code) \
    gameWrapper->SetTimeout([this](GameWrapper* gw) { \
        code \
    }, delaySeconds)

#define DELAY_CAPTURE(delaySeconds, code, ...) \
    gameWrapper->SetTimeout([this, __VA_ARGS__](GameWrapper* gw) { \
        code \
    }, delaySeconds)

#define GAME_THREAD_EXECUTE(code) \
    do { \
        gameWrapper->Execute([this](GameWrapper* gw) { \
            code \
        }); \
    } while (0)

#define GAME_THREAD_EXECUTE_CAPTURE(code, ...) \
    do { \
        gameWrapper->Execute([this, __VA_ARGS__](GameWrapper* gw) { \
            code \
        }); \
    } while (0)

#define RUN_COMMAND(cvar) \
    cvarManager->executeCommand(cvar.name);

#define DELAY_RUN_COMMAND(cvar, delaySeconds) \
    gameWrapper->SetTimeout([this](GameWrapper* gw) { cvarManager->executeCommand(cvar.name); }, delaySeconds);

#endif // MACROS_H