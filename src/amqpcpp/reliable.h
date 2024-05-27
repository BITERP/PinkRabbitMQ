/**
 *  Reliable.h
 *  
 *  A channel wrapper based on AMQP::Tagger that allows message callbacks to be installed
 *  on the publish-confirms, to be called when they a confirmation is received from RabbitMQ.
 * 
 *  You can also change the base class and use Reliable<Throttle> if you not only
 *  want to be notified about the publish-confirms, but want to use it for automatic
 *  throttling at the same time.
 *  
 *  @author Michael van der Werve <michael.vanderwerve@mailerq.com>
 *  @copyright 2020 - 2023 Copernica BV
 */

/**
 *  Header guard
 */
#pragma once

/**
 *  Includes
 */
#include "deferredpublish.h"
#include "tagger.h"
#include <memory>

/**
 *  Begin of namespaces
 */
namespace AMQP { 

/**
 *  Class definition
 */
template <typename BASE=Tagger>
class Reliable : public BASE
{
private:
    // make sure it is a proper channel
    static_assert(std::is_base_of<Tagger, BASE>::value, "base should be derived from a confirmed channel.");

    /**
     *  Set of open deliverytags. We want a normal set (not unordered_set) because
     *  removal will be cheaper for whole ranges.
     *  @var size_t
     */
    std::map<size_t, std::shared_ptr<DeferredPublish>> _handlers;

    /**
     *  Called when the deliverytag(s) are acked
     *  @param  deliveryTag
     *  @param  multiple
     */
    void onAck(uint64_t deliveryTag, bool multiple) override
    {
        // monitor the object, watching for destruction since these ack/nack handlers
        // could destruct the object
        Monitor monitor(this);

        // single element is simple
        if (!multiple)
        {
            // find the element
            auto iter = _handlers.find(deliveryTag);

            // we did not find it (this should not be possible, unless somebody explicitly called)
            // the base-class publish methods for some reason.
            if (iter == _handlers.end()) return BASE::onAck(deliveryTag, multiple);

            // get the handler (we store it first so that we can remove it)
            auto handler = iter->second;

            // erase it from the map (we remove it before the call, because the callback might update
            // the _handlers and invalidate the iterator)
            _handlers.erase(iter);

            // call the ack handler
            handler->reportAck();

        }

        // do multiple at once
        else
        {
            // keep looping for as long as the object is in a valid state
            while (monitor && !_handlers.empty())
            {
                // get the first handler
                auto iter = _handlers.begin();
                
                // make sure this is the right deliverytag, if we've passed it we leap out
                if (iter->first > deliveryTag) break;

                // get the handler
                auto handler = iter->second;
                
                // remove it from the map (before we make a call to userspace, so that user space
                // can add even more handlers, without invalidating iterators)
                _handlers.erase(iter);

                // call the ack handler
                handler->reportAck();
            }
        }

        // make sure the object is still valid
        if (!monitor) return;

        // call base handler as well
        BASE::onAck(deliveryTag, multiple);
    }

    /**
     *  Called when the deliverytag(s) are nacked
     *  @param  deliveryTag
     *  @param  multiple
     */
    void onNack(uint64_t deliveryTag, bool multiple) override
    {
        // monitor the object, watching for destruction since these ack/nack handlers
        // could destruct the object
        Monitor monitor(this);

        // single element is simple
        if (!multiple)
        {
            // find the element
            auto iter = _handlers.find(deliveryTag);

            // we did not find it (this should not be possible, unless somebody explicitly called)
            // the base-class publish methods for some reason.
            if (iter == _handlers.end()) return BASE::onNack(deliveryTag, multiple);

            // get the handler (we store it first so that we can remove it)
            auto handler = iter->second;

            // erase it from the map (we remove it before the call, because the callback might update
            // the _handlers and invalidate the iterator)
            _handlers.erase(iter);

            // call the ack handler
            handler->reportNack();
        }

        // nack multiple elements
        else
        {
            // keep looping for as long as the object is in a valid state
            while (monitor && !_handlers.empty())
            {
                // get the first handler
                auto iter = _handlers.begin();
                
                // make sure this is the right deliverytag, if we've passed it we leap out
                if (iter->first > deliveryTag) break;

                // get the handler
                auto handler = iter->second;
                
                // remove it from the map (before we make a call to userspace, so that user space
                // can add even more handlers, without invalidating iterators)
                _handlers.erase(iter);

                // call the ack handler
                handler->reportNack();
            }
        }

        // if the object is no longer valid, return
        if (!monitor) return;

        // call the base handler
        BASE::onNack(deliveryTag, multiple);
    }

    /**
     *  Method that is called to report an error
     *  @param  message
     */
    void reportError(const char *message) override
    {
        // monitor the object, watching for destruction since these ack/nack handlers
        // could destruct the object
        Monitor monitor(this);

        // move the handlers out
        auto handlers = std::move(_handlers);

        // iterate over all the messages
        // call the handlers
        for (const auto &iter : handlers)
        {
            // call the handler
            iter.second->reportError(message);

            // if we were destructed in the meantime, we leap out
            if (!monitor) return;
        }

        // if the monitor is no longer valid, leap out
        if (!monitor) return;

        // call the base handler
        BASE::reportError(message);
    }

public:
    /**
     *  Constructor
     *  @param  channel
     *  @param  throttle
     */
    template <typename ...Args>
    Reliable(Args &&...args) : BASE(std::forward<Args>(args)...) {}

    /**
     *  Deleted copy constructor, deleted move constructor
     *  @param other
     */
    Reliable(const Reliable &other) = delete;
    Reliable(Reliable &&other) = delete;

    /**
     *  Deleted copy assignment, deleted move assignment
     *  @param  other
     */
    Reliable &operator=(const Reliable &other) = delete;
    Reliable &operator=(Reliable &&other) = delete;

    /**
     *  Virtual destructor
     */
    virtual ~Reliable() = default;

    /**
     *  Method to check how many messages are still unacked.
     *  @return size_t
     */
    virtual size_t unacknowledged() const override { return _handlers.size(); }

    /**
     *  Publish a message to an exchange. See amqpcpp/channel.h for more details on the flags. 
     *  Delays actual publishing depending on the publisher confirms sent by RabbitMQ.
     * 
     *  @param  exchange    the exchange to publish to
     *  @param  routingkey  the routing key
     *  @param  envelope    the full envelope to send
     *  @param  message     the message to send
     *  @param  size        size of the message
     *  @param  flags       optional flags
     *  @return bool
     */
    DeferredPublish &publish(const std::string_view &exchange, const std::string_view &routingKey, const std::string_view &message, int flags = 0) { return publish(exchange, routingKey, Envelope(message.data(), message.size()), flags); }
    DeferredPublish &publish(const std::string_view &exchange, const std::string_view &routingKey, const char *message, size_t size, int flags = 0) { return publish(exchange, routingKey, Envelope(message, size), flags); }
    DeferredPublish &publish(const std::string_view &exchange, const std::string_view &routingKey, const char *message, int flags = 0) { return publish(exchange, routingKey, Envelope(message, strlen(message)), flags); }

    /**
     *  Publish a message to an exchange. See amqpcpp/channel.h for more details on the flags. 
     *  Delays actual publishing depending on the publisher confirms sent by RabbitMQ.
     * 
     *  @param  exchange    the exchange to publish to
     *  @param  routingkey  the routing key
     *  @param  envelope    the full envelope to send
     *  @param  message     the message to send
     *  @param  size        size of the message
     *  @param  flags       optional flags
     */
    DeferredPublish &publish(const std::string_view &exchange, const std::string_view &routingKey, const Envelope &envelope, int flags = 0)
    {
        // publish the entire thing, and remember if it failed at any point
        uint64_t tag = BASE::publish(exchange, routingKey, envelope, flags);
        
        // create the publish deferred object, if we got no tag we failed
        auto handler = std::make_shared<DeferredPublish>(tag == 0);

        // add it to the open handlers
        _handlers[tag] = handler;

        // return the dereferenced handler 
        return *handler;
    }
};

/**
 *  End of namespaces
 */
} 
