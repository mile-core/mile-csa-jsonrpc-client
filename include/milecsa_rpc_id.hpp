//
// Created by denn on 04/10/2018.
//

#pragma once

namespace milecsa {
    namespace rpc {

        template <typename T>
        /**
         * Singleton ID counter
         * @tparam T
         * */
        class IdCounter {

        public:

            /**
             * Get next ID
             * @return next ID
             */
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
