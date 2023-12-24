#include <memory>
#include <variant>
#include <map>
#include <set>
#include <vector>
#include <cmath>

enum class FixpointOperation
{
    division,
    multiplication,
    addition,
    subtraction,
    next_layer_equivalence,

    // Special Function
    ceil,
    floor,
};

template<typename T>
struct FixpointComputation;

template<typename T>
struct Fixpoint
{
public:
    T value;

public:
    Fixpoint(const T& rhs)
        : value(rhs)
    {
    }

    Fixpoint(const Fixpoint& rhs)
        : value(rhs.value)
    {
    }

    FixpointComputation<T> operator=(const FixpointComputation<T>& rhs);
};

template<typename T>
struct FixpointCache
{
public:
    std::map<Fixpoint<T>*, T> cacheFixpoints;

public:
    FixpointCache() = default;

public:
    bool contains(Fixpoint<T>* rhs) const
    {
        return cacheFixpoints.find(rhs) != cacheFixpoints.end();
    }

    T get(Fixpoint<T>* rhs) const
    {
        return cacheFixpoints.find(rhs)->second;
    }

    void remember(Fixpoint<T>* lhs, T rhs)
    {
        if (contains(lhs))
        {
            cacheFixpoints.find(lhs)->second = rhs;
        }
        else
        {
            cacheFixpoints.insert({lhs, rhs});
        }
    }
};

template<typename T>
struct FixpointReference
{
public:
    std::variant<Fixpoint<T>*, T> value;

public:
    FixpointReference(Fixpoint<T>& value_)
        : value(&value_)
    {
    }

    FixpointReference(Fixpoint<T>* value_)
        : value(value_)
    {
    }

    FixpointReference(const T& value_)
        : value(value_)
    {
    }

public:
    T ToT() const
    {
        if (std::holds_alternative<Fixpoint<T>*>(value))
        {
            return std::get<Fixpoint<T>*>(value)->value;
        }
        else if (std::holds_alternative<T>(value))
        {
            return std::get<T>(value);
        }

        throw std::logic_error("Unsupported or invalid type.");
    }

    T  ToT(FixpointCache<T>& cache) const
    {
        if (std::holds_alternative<Fixpoint<T>*>(value))
        {
            if (cache.contains(std::get<Fixpoint<T>*>(value)))
            {
                return cache.get(std::get<Fixpoint<T>*>(value));
            }

            cache.remember(std::get<Fixpoint<T>*>(value), std::get<Fixpoint<T>*>(value)->value);
            return cache.get(std::get<Fixpoint<T>*>(value));
        }
        else if (std::holds_alternative<T>(value))
        {
            return std::get<T>(value);
        }

        throw std::logic_error("Unsupported or invalid type.");
    }
};

template<typename T>
struct FixpointComputation
{
public:
    // std::vector<FixpointReference<T>> variables;
    // std::vector<FixpointOperation> operations;
    std::vector<std::variant<FixpointReference<T>, FixpointComputation<T>>> children;
    FixpointOperation operation;

public:
    FixpointComputation() = default;

public:
    T operator()()
    {
        FixpointCache<T> cache;
        return Computation(cache);
    }

    T Computation(FixpointCache<T>& cache)
    {
        auto LocalParameterComputation = [&](std::size_t index) {
            if (std::holds_alternative<FixpointReference<T>>(children[index]))
            {
                return std::get<FixpointReference<T>>(children[index]).ToT(cache);
            }
            else if (std::holds_alternative<FixpointComputation<T>>(children[index]))
            {
                return std::get<FixpointComputation<T>>(children[index]).Computation(cache);
            }

            throw std::logic_error("Unsupported or invalid type.");
        };

        switch (operation)
        {
        case FixpointOperation::division: {
            return LocalParameterComputation(0) / LocalParameterComputation(1);
        }
        case FixpointOperation::multiplication: {
            return LocalParameterComputation(0) * LocalParameterComputation(1);
        }
        case FixpointOperation::addition: {
            return LocalParameterComputation(0) + LocalParameterComputation(1);
        }
        case FixpointOperation::subtraction: {
            return LocalParameterComputation(0) - LocalParameterComputation(1);
        }
        case FixpointOperation::ceil: {
            return std::ceil(LocalParameterComputation(0));
        }
        case FixpointOperation::floor: {
            return std::floor(LocalParameterComputation(0));
        }
        case FixpointOperation::next_layer_equivalence: {
            T delta = 0.01;
            T newLayer{};
            T  oldLayer{};
            auto fixpoint = std::get<Fixpoint<T>*>(std::get<FixpointReference<T>>(children[0]).value);
            do
            {
                newLayer = LocalParameterComputation(1);
                oldLayer = cache.get(fixpoint);
                fixpoint->value = newLayer;
                cache.remember(fixpoint, newLayer);
                //std::cout << "Old: " << oldLayer << " New: " << newLayer << "\n";
            } while(std::abs(newLayer - oldLayer) > delta);
            return fixpoint->value;
        }
        }

        throw std::logic_error("Invalid operation.");

    }
};

template<typename T>
FixpointComputation<T> operator/(const FixpointComputation<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointComputation<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointReference<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointReference<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointReference<T>& lhs, const Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(Fixpoint<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const FixpointComputation<T>& lhs, const Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(Fixpoint<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(Fixpoint<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator/(const T& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::division;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointComputation<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointComputation<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointReference<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointReference<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointReference<T>& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(Fixpoint<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointComputation<T>& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(Fixpoint<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(Fixpoint<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const T& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const FixpointComputation<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator+(const T& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::addition;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointComputation<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointComputation<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointReference<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointReference<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointReference<T>& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(Fixpoint<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointComputation<T>& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(Fixpoint<T>& lhs, const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(Fixpoint<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> operator*(const T& lhs, Fixpoint<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::multiplication;
    return newComputation;
}

template<typename T>
FixpointComputation<T> Fixpoint<T>::operator=(const FixpointComputation<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(this);
    newComputation.children.push_back(rhs);
    newComputation.operation = FixpointOperation::next_layer_equivalence;
    return newComputation;
}

template<typename T>
struct FixpointSpecialCeil
{
public:
    FixpointComputation<T> value;

public:
    FixpointSpecialCeil(const FixpointComputation<T>& value_)
        : value(value_)
    {
    }
};

template<typename T>
FixpointComputation<T> operator*(const FixpointSpecialCeil<T>& lhs, const FixpointReference<T>& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs.value);
    newComputation.operation = FixpointOperation::ceil;

    auto newNewComputation = FixpointComputation<T>();
    newNewComputation.children.push_back(newComputation);
    newNewComputation.children.push_back(rhs);
    newNewComputation.operation = FixpointOperation::multiplication;
    return newNewComputation;
}

template<typename T>
FixpointComputation<T> operator*(const FixpointSpecialCeil<T>& lhs, const T& rhs)
{
    auto newComputation = FixpointComputation<T>();
    newComputation.children.push_back(lhs.value);
    newComputation.operation = FixpointOperation::ceil;

    auto newNewComputation = FixpointComputation<T>();
    newNewComputation.children.push_back(newComputation);
    newNewComputation.children.push_back(rhs);
    newNewComputation.operation = FixpointOperation::multiplication;
    return newNewComputation;
}