#ifndef HND_HAND_STREAM_H
#define HND_HAND_STREAM_H

#include <astra_core/Plugins/SingleBinStream.h>
#include <astra/capi/streams/hand_types.h>
#include <astra/capi/astra_ctypes.h>
#include <astra/capi/streams/stream_types.h>
#include <Shiny.h>

namespace astra { namespace hand {

    class handstream : public plugins::SingleBinStream<astra_handframe_wrapper_t>
    {
    public:
        handstream(PluginServiceProxy& pluginService,
                   astra_streamset_t streamSet,
                   size_t maxHandCount)
            : SingleBinStream(pluginService,
                              streamSet,
                              stream_description(ASTRA_STREAM_HAND,
                                                DEFAULT_SUBTYPE),
                              sizeof(astra_handpoint_t) * maxHandCount)
        { }

        bool include_candidate_points() const { return includeCandidatePoints_; }
        void set_include_candidate_points(bool includeCandidatePoints)
        {
            includeCandidatePoints_ = includeCandidatePoints;
        }
    protected:
        virtual void on_set_parameter(astra_streamconnection_t connection,
                                      astra_parameter_id id,
                                      size_t inByteLength,
                                      astra_parameter_data_t inData) override;

        virtual void on_get_parameter(astra_streamconnection_t connection,
                                      astra_parameter_id id,
                                      astra_parameter_bin_t& parameterBin) override;
    private:
        void get_include_candidates(astra_parameter_bin_t& parameterBin);
        void set_include_candidates(size_t inByteLength, astra_parameter_data_t& inData);

        virtual void on_connection_removed(astra_bin_t bin,
                                           astra_streamconnection_t connection) override
        {
            SingleBinStream::on_connection_removed(bin, connection);

#ifdef __ANDROID__
            PROFILE_UPDATE();
            PROFILE_OUTPUT("/sdcard/profile_orbbec_hand.txt");
#endif
        }

        bool includeCandidatePoints_{false};
    };

}}

#endif /* HND_HAND_STREAM_H */
