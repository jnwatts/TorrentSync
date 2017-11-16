#include "types.h"
#include "client.h"
#include "json-rpc.h"
#include "task.h"

void register_types()
{
    qRegisterMetaType<JsonRpc>();
//    qRegisterMetaType<Client>();
    qRegisterMetaType<Task::State>();
}
