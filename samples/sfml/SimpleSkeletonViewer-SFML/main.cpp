#include <SFML/Graphics.hpp>
#include <astra/astra.hpp>
#include <iostream>

class skeletonframeListener : public astra::frame_listener
{
public:
    void init_texture(int width, int height)
    {
        if (displayBuffer_ == nullptr || width != depthWidth_ || height != depthHeight_)
        {
            depthWidth_ = width;
            depthHeight_ = height;
            int byteLength = depthWidth_ * depthHeight_ * 4;

            displayBuffer_ = BufferPtr(new uint8_t[byteLength]);
            memset(displayBuffer_.get(), 0, byteLength);

            texture_.create(depthWidth_, depthHeight_);
            sprite_.setTexture(texture_);
            sprite_.setPosition(0, 0);
        }
    }

    void check_fps()
    {
        double fpsFactor = 0.02;

        std::clock_t newTimepoint= std::clock();
        long double frameDuration = (newTimepoint - lastTimepoint_) / static_cast<long double>(CLOCKS_PER_SEC);

        frameDuration_ = frameDuration * fpsFactor + frameDuration_ * (1 - fpsFactor);
        lastTimepoint_ = newTimepoint;
        double fps = 1.0 / frameDuration_;

        printf("FPS: %3.1f (%3.4Lf ms)\n", fps, frameDuration_ * 1000);
    }

    void processDepth(astra::frame& frame)
    {
        astra::depthframe depthFrame = frame.get<astra::depthframe>();

        int width = depthFrame.resolutionX();
        int height = depthFrame.resolutionY();

        init_texture(width, height);

        const int16_t* depthPtr = depthFrame.data();
        for(int y = 0; y < height; y++)
        {
            for(int x = 0; x < width; x++)
            {
                int index = (x + y * width);
                int index4 = index * 4;

                int16_t depth = depthPtr[index];
                uint8_t value = depth % 255;

                displayBuffer_[index4] = value;
                displayBuffer_[index4 + 1] = value;
                displayBuffer_[index4 + 2] = value;
                displayBuffer_[index4 + 3] = 255;
            }
        }

        texture_.update(displayBuffer_.get());
    }

    void processSkeletons(astra::frame& frame)
    {
        astra::skeletonframe skeletonFrame = frame.get<astra::skeletonframe>();

        skeletons_ = skeletonFrame.skeletons();
        jointPositions_.clear();

        for (auto skeleton : skeletons_)
        {
            for(auto joint : skeleton.joints())
            {
                auto depthPosition =
                    mapper_->convert_world_to_depth(joint.position());

                jointPositions_.push_back(depthPosition);
            }
        }
    }

    virtual void on_frame_ready(astra::Stream_Reader& reader,
                                astra::frame& frame) override
    {
        if (mapper_ == nullptr)
        {
            auto& mapper = reader.stream<astra::depthstream>().coordinateMapper();
            mapper_ = std::make_unique<astra::coordinate_mapper>(mapper);
        }

        processDepth(frame);
        processSkeletons(frame);

        check_fps();
    }

    void drawCircle(sf::RenderWindow& window, float radius, float x, float y, sf::Color color)
    {
        sf::CircleShape shape(radius);

        shape.setFillColor(color);

        shape.setOrigin(radius, radius);
        shape.setPosition(x, y);
        window.draw(shape);
    }

    void drawSkeletons(sf::RenderWindow& window, float depthScale)
    {
        float radius = 16;
        sf::Color trackingColor(10, 10, 200);

        for (auto position : jointPositions_)
        {
            drawCircle(window,
                       radius,
                       position.x * depthScale,
                       position.y * depthScale,
                       trackingColor);
        }
    }

    void drawTo(sf::RenderWindow& window)
    {
        if (displayBuffer_ != nullptr)
        {
            float depthScale = window.getView().getSize().x / depthWidth_;

            sprite_.setScale(depthScale, depthScale);

            window.draw(sprite_);

            drawSkeletons(window, depthScale);
        }
    }

private:
    long double frameDuration_{ 0 };
    std::clock_t lastTimepoint_ { 0 };
    sf::Texture texture_;
    sf::Sprite sprite_;

    using BufferPtr = std::unique_ptr < uint8_t[] >;
    BufferPtr displayBuffer_{ nullptr };

    std::unique_ptr<astra::coordinate_mapper> mapper_;
    std::vector<astra::skeleton> skeletons_;
    std::vector<astra::vector3f> jointPositions_;

    int depthWidth_{0};
    int depthHeight_{0};
};

int main(int argc, char** argv)
{
    astra::Astra::initialize();

    sf::RenderWindow window(sf::VideoMode(1280, 960), "Skeleton Viewer");

    astra::streamset sensor;
    astra::stream_reader reader = sensor.create_reader();

    skeletonframeListener listener;

    reader.stream<astra::depthstream>().start();
    reader.stream<astra::skeletonstream>().start();
    reader.addListener(listener);

    while (window.isOpen())
    {
        astra_temp_update();

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape))
                window.close();
        }

        // clear the window with black color
        window.clear(sf::Color::Black);

        listener.drawTo(window);
        window.display();
    }

    astra::Astra::terminate();

    return 0;
}
