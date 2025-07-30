NAME		= ircserv
SRCDIR		= src
SRCS		= main.cpp Server.cpp Client.cpp
SRCS		:= $(addprefix $(SRCDIR)/, $(SRCS))
INCDIR		= inc
INCS		= ft_irc.hpp
INCS		:= $(addprefix $(INCDIR)/, $(INCS))
OBJDIR		= .obj
OBJS		= $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
RM			= rm -rf
CPPFLAGS	= -Wall -Wextra -Werror -std=c++98 -g3
CPPC		= c++

$(OBJDIR):
	@mkdir $(OBJDIR)

$(NAME): $(OBJDIR) $(OBJS) $(INCS)
	$(CPPC) $(CPPFLAGS) $(OBJS) -I$(INCDIR) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
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
