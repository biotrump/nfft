function [x,h_x]=my_rand(M,h)

x=[];
while(length(x)<M)
  X=rand(M^2,1);
  for j=1:length(X)
    is_ok=1;	
    for l=1:length(x)  
      if((abs(X(j)-x(l))<h)||(abs(X(j)+1-x(l))<h)||(abs(X(j)-1-x(l))<h))
	is_ok=0;
      end	
    end
    if(is_ok)
      x=[x;X(j)];
    end
  end
end

x=sort(x(1:M))-0.5;
xx=[x(end)-1;x;x(1)+1];
dist=xx(2:end)-xx(1:end-1);
h_x=min(dist);