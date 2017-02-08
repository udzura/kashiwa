MRUBY_CONFIG=File.expand_path(ENV["MRUBY_CONFIG"] || "build_config.rb")
MRUBY_VERSION=ENV["MRUBY_VERSION"] || "master"

file "mruby" do
  cmd =  "git clone --depth=1 git://github.com/mruby/mruby.git"
  if MRUBY_VERSION != 'master'
    cmd << " && cd mruby"
    cmd << " && git fetch --tags && git checkout $(git rev-parse #{MRUBY_VERSION})"
  end
  sh cmd
end

file "kashiwa" => "src/kashiwa.c" do
  sh "make CFLAGS='-lm' src/kashiwa"
  cp "src/kashiwa", "kashiwa"
end

task :default => "kashiwa"
