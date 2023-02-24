#pragma once

#include <memory>

#define Ref(T) std::shared_ptr<T>
#define New(T, ...) std::make_shared<T>(__VA_ARGS__)
#define Del(T) T.reset()
#define Def(T, ...) struct T : public std::enable_shared_from_this<T>