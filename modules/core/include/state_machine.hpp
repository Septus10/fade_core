#ifndef FADE_CORE_STATE_MACHINE_HPP_
#define FADE_CORE_STATE_MACHINE_HPP_

#include <string>

namespace fade::core {

template <typename StateType>
struct State
{
    State(StateType in_state);
};

template <typename StateType, typename InputType>
struct Transition
{

};

// Contains all the states and transitions for a state machine
template <typename StateType, typename InputType>
class StateMachineConfiguration
{
public:
    StateMachineConfiguration()
    {}

    StateMachineConfiguration(const StateMachineConfiguration& in_other)
    {}

    [[nodiscard]]
    fade::uint16 AddState(explicit State&& in_state);
    [[nodiscard]]
    fade::uint16 AddState(State in_state);
    void AddTransition();

    const std::vector<State>& GetStates();

private:
    // Vector of all the states
    std::vector<State> states_;

    // Vector of all the transitions
    std::map<StateType, Transition> transition_map_;

    // Vector of indices of final states, indices map to the corresponding state in the `states_` member
    std::vector<fade::uint16> final_states_;

    // Index to the initial state, index maps to the corresponding state in the `states_` member
    fade::uint16 initial_state_;
};

// State machine class
// Can be extended to perform additional actions upon entering, remaining in and leaving states.
template <typename StateType, typename InputType>
class StateMachine
{
public:
    constexpr StateMachine(const StateMachineConfiguration& in_state_machine_configuration)
        : state_machine_configuration_(in_state_machine_configuration)
    {}

    bool HandleInput(const InputType& in_input)
    {
        //state_machine_configuration_;
    }

    virtual void OnStateEntered(StateType in_state, InputType in_input) = 0;
    virtual void OnStateRemained(StateType in_state, InputType in_input) = 0;
    virtual void OnStateLeft(StateType in_state, InputType in_input) = 0;

private:
    const StateMachineConfiguration& state_machine_configuration_;
};

}

#endif // FADE_CORE_STATE_MACHINE_HPP_