#include "ps14/logger.h"
int main() { ps14_logger_init(PS14_LOG_LEVEL_INFO, 1, NULL); PS14_LOG_INFO("Server started"); return 0; }
