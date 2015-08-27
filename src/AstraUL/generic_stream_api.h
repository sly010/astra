#ifndef GENERIC_STREAM_API_H
#define GENERIC_STREAM_API_H

#include <Astra/astra_capi.h>
#include <Astra/Plugins/plugin_capi.h>
#include <cassert>
#include <cstring>

template<typename TFrameWrapperType, typename TFrameType>
astra_status_t astra_generic_frame_get(astra_reader_frame_t readerFrame,
                                             astra_stream_type_t type,
                                             astra_stream_subtype_t subtype,
                                             TFrameType** frame)
{
    astra_frame_t* subFrame;
    astra_reader_get_frame(readerFrame, type, subtype, &subFrame);

    TFrameWrapperType* wrapper = reinterpret_cast<TFrameWrapperType*>(subFrame->data);
    *frame = reinterpret_cast<TFrameType*>(&(wrapper->frame));
    (*frame)->frame = subFrame;

    return ASTRA_STATUS_SUCCESS;
}

template<typename TFrameType>
astra_status_t astra_generic_frame_get(astra_reader_frame_t readerFrame,
                                             astra_stream_type_t type,
                                             astra_stream_subtype_t subtype,
                                             TFrameType** frame)
{
    astra_frame_t* subFrame;
    astra_reader_get_frame(readerFrame, type, subtype, &subFrame);

    *frame = reinterpret_cast<TFrameType*>(subFrame->data);
    (*frame)->frame = subFrame;

    return ASTRA_STATUS_SUCCESS;
}

template<typename TFrameType>
astra_status_t astra_generic_frame_get_frameindex(TFrameType* frame,
                                                        astra_frame_index_t* index)
{
    *index = frame->frame->frameIndex;

    return ASTRA_STATUS_SUCCESS;
}


template<typename TElementType>
astra_status_t astra_generic_stream_request_array(astra_streamconnection_t connection,
                                                        astra_parameter_id parameterId,
                                                        astra_result_token_t* token,
                                                        size_t* count)
{
    size_t paramSize;
    astra_status_t rc = astra_stream_get_parameter(connection,
                                                         parameterId,
                                                         &paramSize,
                                                         token);

    *count = paramSize / sizeof(TElementType);

    return rc;
}

template<typename TElementType>
astra_status_t astra_generic_stream_get_result_array(astra_streamconnection_t connection,
                                                           astra_result_token_t token,
                                                           void* array,
                                                           size_t count)
{
    size_t resultSize = count * sizeof(TElementType);

    return astra_stream_get_result(connection,
                                      token,
                                      resultSize,
                                      array);
}

inline astra_status_t astra_stream_get_parameter_fixed(astra_streamconnection_t connection,
                                                             astra_parameter_id parameterId,
                                                             size_t byteLength,
                                                             astra_parameter_data_t* data)
{
    astra_result_token_t token;
    size_t paramSize;
    astra_status_t rc = astra_stream_get_parameter(connection,
                                                         parameterId,
                                                         &paramSize,
                                                         &token);

    if (rc != ASTRA_STATUS_SUCCESS)
    {
        memset(data, 0, byteLength);
        return rc;
    }

    assert(paramSize == byteLength);

    return astra_stream_get_result(connection,
                                      token,
                                      byteLength,
                                      data);
}



#endif /* GENERIC_STREAM_API_H */