#ifndef ASTRA_STREAM_READER_HPP
#define ASTRA_STREAM_READER_HPP

#include "capi/astra_core.h"
#include <astra_core/astra_frame_listener.hpp>
#include <astra_core/astra_frame.hpp>
#include <memory>
#include <vector>
#include <algorithm>
#include <functional>

namespace astra {

    class stream_reader
    {
    public:
        stream_reader(astra_reader_t reader)
            : readerRef_(std::make_shared<reader_ref>(reader))
        {}

        template<typename T>
        T stream()
        {
            return stream<T>(DEFAULT_SUBTYPE);
        }

        template<typename T>
        T stream(astra_stream_subtype_t subtype)
        {
            astra_streamconnection_t connection;

            astra_reader_get_stream(readerRef_->get_reader(),
                                    T::id,
                                    subtype,
                                    &connection);

            return T(connection);
        }

        void addListener(frame_listener& listener)
        {
            readerRef_.get()->addListener(listener);
        }

        void removeListener(frame_listener& listener)
        {
            readerRef_.get()->removeListener(listener);
        }

        bool is_valid() { return readerRef_ != nullptr; }

        frame get_latest_frame(int timeoutMillis = ASTRA_TIMEOUT_FOREVER)
        {
            astra_reader_frame_t frame;
            astra_reader_open_frame(readerRef_->get_reader(), timeoutMillis, &frame);

            return astra::frame(frame);
        }

    private:
        class reader_ref;
        using reader_ref_ptr = std::shared_ptr<reader_ref>;

        stream_reader(reader_ref_ptr readerRef)
            : readerRef_(readerRef)
        { }

        class reader_ref :
            public std::enable_shared_from_this<reader_ref>
        {
        public:
            reader_ref(astra_reader_t reader)
                :  reader_(reader)
            { }

            ~reader_ref()
            {
                listeners_.clear();
                ensure_callback_removed();
                astra_reader_destroy(&reader_);
            }

            static void frame_ready_thunk(void* clientTag,
                                          astra_reader_t reader,
                                          astra_reader_frame_t frame)
            {
                reader_ref* self = static_cast<reader_ref*>(clientTag);
                self->notify_listeners(frame);
            }

            void addListener(frame_listener& listener)
            {
                ensure_callback_added();

                auto it = std::find(listeners_.begin(),
                                    listeners_.end(),
                                    listener);

                if (it != listeners_.end())
                    return;

                if (isNotifying_)
                {
                    addedListeners_.push_back(listener);
                }
                else
                {
                    listeners_.push_back(listener);
                }
            }

            void removeListener(frame_listener& listener)
            {
                auto it = std::find(listeners_.begin(),
                                    listeners_.end(),
                                    listener);

                if (it == listeners_.end())
                    return;

                if (isNotifying_)
                {
                    removedListeners_.push_back(listener);
                }
                else
                {
                    listeners_.erase(it);
                }

                if (listeners_.size() == 0)
                {
                    ensure_callback_removed();
                }
            }

            void notify_listeners(astra_reader_frame_t readerFrame)
            {
                if (removedListeners_.size() > 0)
                {
                    for(frame_listener& listener : removedListeners_)
                    {
                        auto it = std::find(listeners_.begin(),
                                            listeners_.end(),
                                            listener);

                        listeners_.erase(it);
                    }
                    removedListeners_.clear();
                }

                std::move(addedListeners_.begin(),
                          addedListeners_.end(),
                          std::back_inserter(listeners_));

                if (listeners_.size() == 0)
                {
                    ensure_callback_removed();
                    return;
                }

                //we didn't open the frame, so don't auto close it.
                //the Stream_Reader internals will close it automatically
                const bool autoCloseFrame = false;
                astra::frame frameWrapper(readerFrame, autoCloseFrame);

                isNotifying_ = true;
                stream_reader reader(shared_from_this());
                for(frame_listener& listener : listeners_)
                {
                    listener.on_frame_ready(reader, frameWrapper);
                }
                isNotifying_ = false;
            }

            astra_reader_t get_reader() { return reader_; }

        private:
            void ensure_callback_added()
            {
                if (!callbackRegistered_)
                {
                    astra_reader_register_frame_ready_callback(reader_,
                                                               &reader_ref::frame_ready_thunk,
                                                               this,
                                                               &callbackId_);

                    callbackRegistered_ = true;
                }
            }

            void ensure_callback_removed()
            {
                if (callbackRegistered_)
                {
                    astra_reader_unregister_frame_ready_callback(&callbackId_);
                    callbackRegistered_ = false;
                }
            }

            astra_reader_t reader_;

            bool isNotifying_{false};
            bool callbackRegistered_{false};

            using ListenerList = std::vector<std::reference_wrapper<frame_listener> >;

            ListenerList listeners_;
            ListenerList addedListeners_;
            ListenerList removedListeners_;

            astra_reader_callback_id_t callbackId_;
        };

        reader_ref_ptr readerRef_;

        friend bool operator==(const stream_reader& lhs, const stream_reader& rhs);
    };

    inline bool operator==(const stream_reader& lhs, const stream_reader& rhs)
    {
        return lhs.readerRef_ == rhs.readerRef_;
    }

    inline bool operator!=(const stream_reader& lhs, const stream_reader& rhs)
    {
        return !(lhs == rhs);
    }
}

#endif // ASTRA_STREAM_READER_HPP
