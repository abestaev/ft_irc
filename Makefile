NAME		= ircserv
SRCDIR		= src
CMDDIR		= $(SRCDIR)/commands
BASE_SRCS	= main.cpp Server.cpp Client.cpp Message.cpp Channel.cpp Commands.cpp utils.cpp
SRCS		= $(addprefix $(SRCDIR)/, $(BASE_SRCS)) $(wildcard $(CMDDIR)/*.cpp)
INCDIR		= inc
INCS		= ft_irc.hpp
INCS		:= $(addprefix $(INCDIR)/, $(INCS))
OBJDIR		= .obj
OBJS		= $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
RM			= rm -rf
CPPFLAGS	= -Wall -Wextra -Werror -std=c++98 -g3
CPPC		= c++

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(NAME): $(OBJDIR) $(OBJS) $(INCS)
	$(CPPC) $(CPPFLAGS) $(OBJS) -I$(INCDIR) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CPPC) $(CPPFLAGS) -I$(INCDIR) -o $@ -c $^

all: $(NAME)

clean:
	@echo removing $(OBJS)
	@$(RM) $(OBJS) $(OBJDIR)

fclean: clean
	@echo removing $(NAME)
	@$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re