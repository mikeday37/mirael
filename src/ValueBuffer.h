#pragma once

namespace Mirael
{

/*
 * The ValueBuffer wraps the value at an Output Pin.
 *
 * The Runner assigns each Output Pin its own ValueBuffer.  Each Node Core is responsible for
 * updating the value of the ValueBuffer for each of its Output Pins.  It can do that directly
 * by calling setValue(), or indirectly by calling setAsReference() and passing it a pointer
 * to the ValueBuffer of one of its Inputs.  For non-modifying data passthrough, setAsReference()
 * should be preferred to copying when dealing with arbitrary values (when implementing switches,
 * junctions, etc.), since values may be very large.
 *
 * This is safe within the calculation of a single frame, because ValueBuffers referenced by
 * Input Pins during a Core's execution will never be modified again during that frame.
 *
 * NOTE: References cannot be relied upon between frames.  They are valid only within the context
 * of a single frame, since the Execution Plan can be updated between frames, resulting in the
 * destruction of ValueBuffers for any removed Output Pins.
 */
class ValueBuffer final
{
public:
    void setValue(std::string &&value)
    {
        reference_ = nullptr;
        value_     = std::move(value);
    }

    void setValue(std::string_view value)
    {
        reference_ = nullptr;
        value_     = value;
    }

    void setAsReference(ValueBuffer *to)
    {
        assert(to);         // it is an error to try to set to nullptr
        assert(to != this); // it is an error to set this as a ref to itself
#ifndef NDEBUG
        value_.clear(); // not strictly necessary but makes debugging clearer
#endif
        reference_ = to->reference_ ? to->reference_ : to;
        assert(!reference_->reference_); // topological ordering of the runner should ensure that this always collapses to no more than
                                         // one level deep
    }

    const std::string &getValue() const { return reference_ ? reference_->value_ : value_; }

    void clear()
    {
        reference_ = nullptr;
        value_.clear();
    }

    void unsetReference() { reference_ = nullptr; }

private:
    ValueBuffer *reference_ =
        nullptr;          // non-owning reference - ValueBuffers are owned by the Runner and remain valid within one Frame.
    std::string value_{}; // this is temporary - for the first test we're basically TCL - everything is a string!  :D
};

} // namespace Mirael