//
// Created by denn on 04/10/2018.
//

#ifndef MILECSA_JSONRPC_MILECSA_ID_COUNTER_HPP
#define MILECSA_JSONRPC_MILECSA_ID_COUNTER_HPP

namespace milecsa {
    namespace rpc {

        template <typename T>
        class IdCounter {

        public:

            T get_next() {
                return id_++;
            }

        public:

            static IdCounter& Instance() {
                static IdCounter myInstance;
                return myInstance;
            }

            IdCounter(IdCounter const&) = delete;             // Copy construct
            IdCounter(IdCounter&&) = delete;                  // Move construct
            IdCounter& operator=(IdCounter const&) = delete;  // Copy assign
            IdCounter& operator=(IdCounter &&) = delete;      // Move assign

        protected:
            IdCounter():id_((T)0) {
            }
            ~IdCounter() {}

        private:
            T id_;

        };
    }
}

#endif //MILECSA_JSONRPC_MILECSA_ID_COUNTER_HPP
