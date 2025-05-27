#******************************************************************************#
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: bazaluga <bazaluga@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/05/27 20:45:45 by bazaluga          #+#    #+#              #
#    Updated: 2025/05/27 21:18:58 by bazaluga         ###   ########.fr        #
#                                                                              #
#******************************************************************************#

NAME		:=	ircserv
SRCDIR	    :=	src
OBJDIR	    :=	.obj
SRC		    :=	main.cpp
OBJ		    :=	$(SRC:.cpp=.o)
SRC		    :=	$(addprefix $(SRCDIR)/, $(SRC))
OBJ		    :=	$(addprefix $(OBJDIR)/, $(OBJ))
CPPFLAGS	:=	-Wall -Wextra -Werror -std=c++98 -MMD
CC			:=	c++

# colors
GREEN		:=	"\033[1;32m"
RED			:=	"\033[1;33m"
RESET		:=	"\033[0m"

all:		$(NAME)

$(NAME):	$(OBJ)
			@$(CC) $(OBJ) -o $(NAME)
			@printf $(GREEN)"🚀 $(NAME) was successfully built.\n\n🏃‍♂️Run ./ircserv <port> <password> to get started.\n"$(RESET)

$(OBJDIR):
			mkdir -p $(OBJDIR)

$(OBJDIR)/%.o:	$(SRCDIR)/%.cpp | $(OBJDIR)
				@echo $(GREEN)"⚙️ Compiling..."$(RESET)
				$(CC) $(CPPFLAGS) -c $< -o $@

-include	$(OBJ:.o=.d)

clean:
		@rm -f $(OBJ)
		@rm -f $(OBJ:.o=.d)
		@printf $(RED)"🗑️ object files deleted.\n"$(RESET)

fclean:	clean
		@rm -f $(NAME)
		@printf $(RED)"❌ You just deleted all the compiler's work...\n"$(RESET)

re:		fclean $(NAME)

.PHONY:	all clean fclean
