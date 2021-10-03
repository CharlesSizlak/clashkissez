#include <stdint.h>
#include "contexts.h"
#include "chess.h"
#include "loop.h"
#include "debug.h"
#include "handlers.h"

void message_handler(loop_t *loop, int fd, connection_ctx_t *connection_ctx)  {
    uint8_t *message = connection_ctx->read_buffer + 4;
    uint32_t message_length = be32toh(*(uint32_t*)connection_ctx->read_buffer);

    request_ctx_t *request_ctx = calloc(1, sizeof(request_ctx_t));
    request_ctx->connection_ctx = connection_ctx;

    if (message_length == 0) {
        DEBUG_PRINTF("Received empty message");
        invalid_packet_handler(loop, fd, request_ctx);
        return;
    }
    
    TableType_e table_type = determine_table_type(message, message_length);
    switch (table_type) {
        case TABLE_TYPE_LoginRequest: {
            DEBUG_PRINTF("Received login request");
            request_ctx->message = LoginRequest_deserialize(message, message_length);
            login_request_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_RegisterRequest: {
            DEBUG_PRINTF("Received register request");
            request_ctx->message = RegisterRequest_deserialize(message, message_length);
            register_request_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_UserLookupRequest: {
            DEBUG_PRINTF("UserLookup request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameInviteRequest: {
            DEBUG_PRINTF("GameInvite request");
            request_ctx->message = GameInviteRequest_deserialize(message, message_length);
            game_invite_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameAcceptRequest: {
            DEBUG_PRINTF("GameAccept request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FullGameInformationRequest: {
            DEBUG_PRINTF("FullGameInformation request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameMoveRequest: {
            DEBUG_PRINTF("GameMove request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameLastMoveRequest: {
            DEBUG_PRINTF("GameLastMove request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameHeartbeatRequest: {
            DEBUG_PRINTF("GameHeartbeatRequest request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameDrawOfferRequest: {
            DEBUG_PRINTF("GameDrawOffer request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameDrawOfferResponse: {
            DEBUG_PRINTF("GameDrawOffer response not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameResignationRequest: {
            DEBUG_PRINTF("GameResignationRequest request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequest: {
            DEBUG_PRINTF("Friend request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequestResponse: {
            DEBUG_PRINTF("FriendRequest response not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequestStatusRequest: {
            DEBUG_PRINTF("FriendRequestStatus request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_ActiveGameRequest: {
            DEBUG_PRINTF("ActiveGame request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameHistoryRequest: {
            DEBUG_PRINTF("GameHistory request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_PastGameFullInformationRequest: {
            DEBUG_PRINTF("PastGameFullInformation request not implemented");
            not_implemented_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameInviteSubscribe: {
            DEBUG_PRINTF("GameInviteSubscribe request");
            request_ctx->message = GameInviteSubscribe_deserialize(message, message_length);
            game_invite_subscribe_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameAcceptSubscribe: {
            DEBUG_PRINTF("GameAcceptSubscribe request");
            request_ctx->message = GameAcceptSubscribe_deserialize(message, message_length);
            game_accept_subscribe_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_GameSubscribe: {
            DEBUG_PRINTF("GameSubscribe request");
            request_ctx->message = GameSubscribe_deserialize(message, message_length);
            game_subscribe_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequestSubscribe: {
            DEBUG_PRINTF("FriendRequestSubscribe request");
            request_ctx->message = FriendRequestSubscribe_deserialize(message, message_length);
            friend_request_subscribe_handler(loop, fd, request_ctx);
        }
        break;
        case TABLE_TYPE_FriendRequestAcceptedSubscribe: {
            DEBUG_PRINTF("FriendRequestAcceptedSubscribe request");
            request_ctx->message = FriendRequestAcceptedSubscribe_deserialize(message, message_length);
            friend_request_accepted_subscribe_handler(loop, fd, request_ctx);
        }
        break;
        default: {
            invalid_packet_handler(loop, fd, request_ctx);
        }
    }

    // Remove bytes from the read buffer
    memmove(
        connection_ctx->read_buffer, 
        connection_ctx->read_buffer + message_length + 4, 
        connection_ctx->total_bytes_read - (message_length + 4)
    );
    connection_ctx->total_bytes_read -= message_length + 4;
}
