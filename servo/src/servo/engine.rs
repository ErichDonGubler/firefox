import gfx::renderer::{Renderer, Sink};
import pipes::{spawn_service, select};
import layout::layout_task;
import layout_task::Layout;
import content::{Content, ExecuteMsg, ParseMsg, ExitMsg, create_content};
import resource::resource_task;
import resource::resource_task::{ResourceTask};
import std::net::url::url;
import resource::image_cache_task;
import image_cache_task::{ImageCacheTask, image_cache_task, ImageCacheTaskClient};

import pipes::{port, chan};

fn macros() {
    include!("macros.rs");
}

struct Engine<S:Sink send copy> {
    let sink: S;

    let renderer: Renderer;
    let resource_task: ResourceTask;
    let image_cache_task: ImageCacheTask;
    let layout: Layout;
    let content: comm::Chan<content::ControlMsg>;

    new(+sink: S) {
        self.sink = sink;

        let renderer = Renderer(sink);
        let resource_task = ResourceTask();
        let image_cache_task = image_cache_task(resource_task);
        let layout = Layout(renderer, image_cache_task);
        let content = create_content(layout, sink, resource_task);

        self.renderer = renderer;
        self.resource_task = resource_task;
        self.image_cache_task = image_cache_task;
        self.layout = layout;
        self.content = content;
    }

    fn start() -> EngineProto::client::Running {
        do spawn_service(EngineProto::init) |request| {
            // this could probably be an @vector
            let mut request = ~[move request];

            loop {
                match move select(move request) {
                  (_, some(ref message), ref requests) => {
                    match move self.handle_request(move_ref!(message)) {
                      some(ref req) =>
                          request = vec::append_one(move_ref!(requests),
                                                    move_ref!(req)),
                      none => break
                    }
                  }

                  _ => fail ~"select returned something unexpected."
                }
            }
        }
    }

    fn handle_request(+request: EngineProto::Running)
        -> option<EngineProto::server::Running>
    {
        import EngineProto::*;
        match move request {
          LoadURL(ref url, ref next) => {
            // TODO: change copy to move once we have match move
            let url = move_ref!(url);
            if url.path.ends_with(".js") {
                self.content.send(ExecuteMsg(url))
            } else {
                self.content.send(ParseMsg(url))
            }
            return some(move_ref!(next));
          }

          Exit(ref channel) => {
            self.content.send(content::ExitMsg);
            self.layout.send(layout_task::ExitMsg);
            
            let (response_chan, response_port) = pipes::stream();

            self.renderer.send(renderer::ExitMsg(response_chan));
            response_port.recv();

            self.image_cache_task.exit();
            self.resource_task.send(resource_task::Exit);

            server::Exited(move_ref!(channel));
            return none;
          }
        }
    }
}

proto! EngineProto {
    Running:send {
        LoadURL(url) -> Running,
        Exit -> Exiting
    }

    Exiting:recv {
        Exited -> !
    }
}
